/**
 * @file lib_base.cpp
 * @brief Implementation of TestBase class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "lib_base.h"
#include "platform_rpi5.h"
#include <chrono>
#include <iostream>
#include <stdexcept>

TestBase::TestBase() : stopped_(true)
{
}

TestBase::~TestBase()
{
    stop();
}

// ---------------------------------------------------------------------------
// Internal: exception-safe wrapper around runAllTests().
// Returns a single ERROR result if any exception escapes the derived class.
// ---------------------------------------------------------------------------
std::vector<TestResult> TestBase::runAllTestsSafe()
{
    try {
        return runAllTests();
    } catch (const std::exception& ex) {
        TestResult err;
        err.name    = "runAllTests_exception";
        err.test_id = err.name;
        err.output.status        = hal::Status::ERROR;
        err.output.severity      = hal::Severity::CRITICAL;
        err.output.error_message = std::string("Exception in runAllTests: ") + ex.what();
        std::cerr << "[TestBase] " << err.output.error_message << "\n";
        return {err};
    } catch (...) {
        TestResult err;
        err.name    = "runAllTests_exception";
        err.test_id = err.name;
        err.output.status        = hal::Status::ERROR;
        err.output.severity      = hal::Severity::CRITICAL;
        err.output.error_message = "Unknown exception in runAllTests";
        std::cerr << "[TestBase] " << err.output.error_message << "\n";
        return {err};
    }
}

// Internal: wait while paused (checked at the start of each background cycle).
void TestBase::waitWhilePaused()
{
    while (paused_.load() && !stopped_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// ---------------------------------------------------------------------------
// SINGLE_SHOT / MONITORING (backward-compatible)
// ---------------------------------------------------------------------------

void TestBase::runTests(TestMode mode,
                        std::function<void(const std::vector<TestResult>&)> callback,
                        int interval_ms)
{
    if (mode == TestMode::SINGLE_SHOT) {
        auto results = runAllTestsSafe();
        if (callback) {
            callback(results);
        }
    } else if (mode == TestMode::MONITORING) {
        stop();
        stopped_ = false;
        monitoring_thread_ = std::thread([this, callback, interval_ms]() {
            while (!stopped_) {
                waitWhilePaused();
                if (stopped_) break;
                auto results = runAllTestsSafe();
                if (callback) {
                    callback(results);
                }
                auto wake = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval_ms);
                while (!stopped_ && std::chrono::steady_clock::now() < wake) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
            }
        });
    }
}

// ---------------------------------------------------------------------------
// LONG_TERM — fixed-duration periodic test
// ---------------------------------------------------------------------------

void TestBase::runLongTerm(int interval_ms, long long duration_ms,
                            std::function<void(const std::vector<TestResult>&, int)> callback)
{
    stop();
    stopped_ = false;
    monitoring_thread_ = std::thread([this, interval_ms, duration_ms, callback]() {
        auto start = std::chrono::steady_clock::now();
        int cycle = 0;
        while (!stopped_) {
            waitWhilePaused();
            if (stopped_) break;
            if (duration_ms > 0) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start).count();
                if (elapsed >= duration_ms) {
                    break;
                }
            }
            auto results = runAllTestsSafe();
            ++cycle;
            if (callback) {
                callback(results, cycle);
            }
            auto wake = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval_ms);
            while (!stopped_ && std::chrono::steady_clock::now() < wake) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        stopped_ = true;
    });
}

// ---------------------------------------------------------------------------
// MONITORING TEST — long-term + hardware monitor sampling
// ---------------------------------------------------------------------------

void TestBase::runMonitoringTest(int interval_ms, long long duration_ms,
                                  const std::vector<MonitorItem>& monitorItems,
                                  std::function<void(const MonitoringSnapshot&)> callback)
{
    stop();
    stopped_ = false;
    monitoring_thread_ = std::thread([this, interval_ms, duration_ms, monitorItems, callback]() {
        auto start = std::chrono::steady_clock::now();
        int cycle = 0;
        while (!stopped_) {
            waitWhilePaused();
            if (stopped_) break;
            if (duration_ms > 0) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start).count();
                if (elapsed >= duration_ms) {
                    break;
                }
            }
            MonitoringSnapshot snap;
            snap.cycle = ++cycle;
            snap.timestamp = std::chrono::system_clock::now();
            snap.testResults = runAllTestsSafe();
            for (const auto& item : monitorItems) {
                std::string val;
                try {
                    val = item.reader();
                } catch (...) {
                    val = "error";
                }
                snap.monitorData.emplace_back(item.name, val);
            }
            if (callback) {
                callback(snap);
            }
            auto wake = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval_ms);
            while (!stopped_ && std::chrono::steady_clock::now() < wake) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        stopped_ = true;
    });
}

// ---------------------------------------------------------------------------
// EVENT TEST — fire callback when predicate is satisfied
// ---------------------------------------------------------------------------

void TestBase::runEventTest(int poll_interval_ms, const EventCriteria& criteria,
                             std::function<void(const std::vector<TestResult>&)> event_callback,
                             long long max_duration_ms)
{
    stop();
    stopped_ = false;
    monitoring_thread_ = std::thread([this, poll_interval_ms, max_duration_ms,
                                       criteria, event_callback]() {
        auto start = std::chrono::steady_clock::now();
        while (!stopped_) {
            waitWhilePaused();
            if (stopped_) break;
            if (max_duration_ms > 0) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start).count();
                if (elapsed >= max_duration_ms) {
                    break;
                }
            }
            auto results = runAllTestsSafe();
            if (criteria.predicate && criteria.predicate(results)) {
                if (event_callback) {
                    event_callback(results);
                }
            }
            auto wake = std::chrono::steady_clock::now() + std::chrono::milliseconds(poll_interval_ms);
            while (!stopped_ && std::chrono::steady_clock::now() < wake) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        stopped_ = true;
    });
}

// ---------------------------------------------------------------------------
// Control
// ---------------------------------------------------------------------------

void TestBase::stop()
{
    stopped_ = true;
    if (monitoring_thread_.joinable()) {
        monitoring_thread_.join();
    }
}

bool TestBase::isRunning() const
{
    return !stopped_;
}

void TestBase::pause()
{
    paused_.store(true);
}

void TestBase::resume()
{
    paused_.store(false);
}

bool TestBase::isPaused() const
{
    return paused_.load();
}

std::vector<TestResult> TestBase::runSelectedTests(const std::vector<std::string>& testNames)
{
    const auto available = getAvailableTests();
    std::vector<TestResult> results;
    results.reserve(testNames.size());
    for (const auto& name : testNames) {
        bool found = false;
        for (const auto& avail : available) {
            if (avail == name) { found = true; break; }
        }
        if (!found) {
            TestResult err;
            err.name    = name;
            err.test_id = name;
            err.output.status        = hal::Status::ERROR;
            err.output.severity      = hal::Severity::ERROR;
            err.output.error_message = "Unknown test name: \"" + name + "\"";
            results.push_back(std::move(err));
            continue;
        }
        try {
            results.push_back(runSingleTest(name));
        } catch (const std::exception& ex) {
            TestResult err;
            err.name    = name;
            err.test_id = name;
            err.output.status        = hal::Status::ERROR;
            err.output.severity      = hal::Severity::CRITICAL;
            err.output.error_message = std::string("Exception in runSingleTest(\"") + name + "\"): " + ex.what();
            results.push_back(std::move(err));
        } catch (...) {
            TestResult err;
            err.name    = name;
            err.test_id = name;
            err.output.status        = hal::Status::ERROR;
            err.output.severity      = hal::Severity::CRITICAL;
            err.output.error_message = "Unknown exception in runSingleTest(\"" + name + "\")";
            results.push_back(std::move(err));
        }
    }
    return results;
}