/**
 * @file test_runner.h
 * @brief Generic CLI-driven test runner template for all HAL test executables
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC *
 * Provides runTestApp<T>() — a single template function that implements all four
 * test modes via command-line arguments.  Each test binary's main.cpp reduces to:
 *
 *     #include "test_runner.h"
 *     #include "my_lib/my_lib.h"
 *     int main(int argc, char* argv[]) {
 *         return runTestApp<MyTesting>("MyTesting", argc, argv);
 *     }
 *
 * CLI usage:
 *   <exe>                                        one-shot (default)
 *   <exe> one-shot                               run all tests once
 *   <exe> monitoring [interval_ms]               run indefinitely at interval (Ctrl+C stops)
 *   <exe> long-term  <interval_ms> <duration>    run for fixed duration
 *   <exe> monitor-test <interval_ms> <duration>  long-term + Pi5 sysfs hardware monitors
 *   <exe> event <poll_ms> <criteria> [max_dur]   fire callback when criteria met
 *
 * Duration formats:  500ms  30s  5m  2h  1d   (plain number = milliseconds)
 * Event criteria:    any-fail  all-fail  all-pass  fail-rate-above-N  (N = 0-100)
 */
#pragma once

#include "lib_base.h"

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// Signal handling (one copy per binary — static is fine here)
// ---------------------------------------------------------------------------

static std::atomic<bool> g_test_stop_flag{false};

static void testRunnerSigHandler(int /*signal*/)
{
    g_test_stop_flag = true;
}

// ---------------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------------

static std::string trReadSysfs(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        return "";
    }
    std::string line;
    std::getline(f, line);
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r' || line.back() == ' ')) {
        line.pop_back();
    }
    return line;
}

// Parse human-friendly duration string to milliseconds.
// Accepted: "500ms"  "30s"  "5m"  "2h"  "1d"  or plain integer (treated as ms).
// Returns -1 if the input cannot be parsed.
static long long trParseDuration(const std::string& s)
{
    try {
        if (s.size() >= 3 && s.substr(s.size() - 2) == "ms") {
            return std::stoll(s.substr(0, s.size() - 2));
        }
        if (s.size() >= 2) {
            char u = s.back();
            std::string n = s.substr(0, s.size() - 1);
            if (u == 's') return std::stoll(n) * 1000LL;
            if (u == 'm') return std::stoll(n) * 60000LL;
            if (u == 'h') return std::stoll(n) * 3600000LL;
            if (u == 'd') return std::stoll(n) * 86400000LL;
        }
        return std::stoll(s); // plain integer → ms
    } catch (...) {
        std::cerr << "[ERROR] Invalid duration string: \"" << s << "\"\n";
        return -1LL;
    }
}

// ---------------------------------------------------------------------------
// Built-in Raspberry Pi 5 hardware monitor items
// ---------------------------------------------------------------------------

static std::vector<MonitorItem> trPi5MonitorItems()
{
    return {
        {"cpu_temp_C", []() {
            std::string v = trReadSysfs("/sys/class/thermal/thermal_zone0/temp");
            if (v.empty()) return std::string("N/A");
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1) << (std::stod(v) / 1000.0);
            return ss.str();
        }},
        {"cpu0_freq_MHz", []() {
            std::string v = trReadSysfs("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
            if (v.empty()) return std::string("N/A");
            return std::to_string(std::stoll(v) / 1000);
        }},
        {"gpu_freq_MHz", []() {
            std::string v = trReadSysfs("/sys/class/devfreq/fde00000.gpu/cur_freq");
            if (v.empty()) return std::string("N/A");
            return std::to_string(std::stoll(v) / 1000000);
        }},
        {"mem_avail_MB", []() {
            std::ifstream f("/proc/meminfo");
            if (!f.is_open()) return std::string("N/A");
            std::string line;
            while (std::getline(f, line)) {
                if (line.rfind("MemAvailable:", 0) == 0) {
                    std::istringstream ss(line.substr(13));
                    long long kb = 0;
                    ss >> kb;
                    return std::to_string(kb / 1024) + " MB";
                }
            }
            return std::string("N/A");
        }},
    };
}

// ---------------------------------------------------------------------------
// EventCriteria factory from a CLI keyword string
// ---------------------------------------------------------------------------

static EventCriteria trMakeCriteria(const std::string& spec)
{
    if (spec == "any-fail") {
        return {"any test fails",
                [](const std::vector<TestResult>& r) {
                    for (const auto& t : r) {
                        if (t.output.status != hal::Status::PASS) return true;
                    }
                    return false;
                }};
    }
    if (spec == "all-fail") {
        return {"all tests fail",
                [](const std::vector<TestResult>& r) {
                    for (const auto& t : r) {
                        if (t.output.status == hal::Status::PASS) return false;
                    }
                    return !r.empty();
                }};
    }
    if (spec == "all-pass") {
        return {"all tests pass",
                [](const std::vector<TestResult>& r) {
                    for (const auto& t : r) {
                        if (t.output.status != hal::Status::PASS) return false;
                    }
                    return !r.empty();
                }};
    }
    // fail-rate-above-N  (N is integer percentage 0–100)
    if (spec.rfind("fail-rate-above-", 0) == 0) {
        int threshold = 0;
        try {
            threshold = std::stoi(spec.substr(16));
        } catch (...) {
            std::cerr << "[ERROR] Invalid fail-rate threshold in '" << spec << "'\n";
            return {spec, [](const std::vector<TestResult>&) { return false; }};
        }
        if (threshold < 0 || threshold > 100) {
            std::cerr << "[ERROR] fail-rate-above-N threshold must be 0–100, got " << threshold << "\n";
            return {spec, [](const std::vector<TestResult>&) { return false; }};
        }
        return {"fail rate above " + std::to_string(threshold) + "%",
                [threshold](const std::vector<TestResult>& r) {
                    if (r.empty()) return false;
                    int fails = 0;
                    for (const auto& t : r) {
                        if (t.output.status != hal::Status::PASS) ++fails;
                    }
                    return (fails * 100 / static_cast<int>(r.size())) > threshold;
                }};
    }
    std::cerr << "[WARNING] Unknown event criteria '" << spec
              << "'. Valid: any-fail | all-fail | all-pass | fail-rate-above-N\n";
    return {spec, [](const std::vector<TestResult>&) { return false; }};
}

// ---------------------------------------------------------------------------
// Output helpers
// ---------------------------------------------------------------------------

static void trPrintTable(const std::vector<TestResult>& results)
{
    int passed = 0, failed = 0, errored = 0, skipped = 0;
    std::chrono::duration<double> total{};
    std::cout << "\n"
              << std::left << std::setw(42) << "Test Name"
              << std::setw(9) << "Result"
              << "Duration(s)\n"
              << std::string(64, '-') << "\n";
    for (const auto& r : results) {
        const char* label = "FAIL";
        switch (r.output.status) {
            case hal::Status::PASS:    label = "PASS";    ++passed;  break;
            case hal::Status::FAIL:    label = "FAIL";    ++failed;  break;
            case hal::Status::ERROR:   label = "ERROR";   ++errored; break;
            case hal::Status::TIMEOUT: label = "TIMEOUT"; ++errored; break;
            case hal::Status::SKIPPED: label = "SKIP";    ++skipped; break;
            case hal::Status::ABORTED: label = "ABORT";   ++errored; break;
        }
        std::cout << std::left << std::setw(42) << r.name
                  << std::setw(9) << label
                  << std::fixed << std::setprecision(6)
                  << std::chrono::duration<double>(r.duration).count() << "\n";
        for (const auto& kv : r.output.reason) {
            std::cout << "    [" << kv.first << "] " << kv.second << "\n";
        }
        if (!r.output.error_message.empty()) {
            std::cout << "    [error] " << r.output.error_message << "\n";
        }
        total += r.duration;
    }
    std::cout << std::string(64, '-') << "\n"
              << "Total: " << results.size()
              << "  Pass: " << passed
              << "  Fail: " << failed;
    if (errored > 0) std::cout << "  Error: " << errored;
    if (skipped > 0) std::cout << "  Skip: "  << skipped;
    std::cout << "  Time: " << std::fixed << std::setprecision(4) << total.count() << "s\n";
}

static void trPrintCycleHeader(int cycle, const std::string& label, bool clear_screen)
{
    if (clear_screen) {
        // Use ANSI escape sequence instead of system("clear") to avoid
        // spawning a shell process and the associated OWASP A03 risk.
        std::cout << "\033[2J\033[H";
    }
    std::cout << "\n[Cycle " << cycle << "] " << label << "\n";
}

static void trPrintMonitorData(const std::vector<std::pair<std::string, std::string>>& data)
{
    if (data.empty()) return;
    std::cout << "--- System Monitors ---\n";
    for (const auto& kv : data) {
        std::cout << "  " << std::left << std::setw(20) << kv.first << ": " << kv.second << "\n";
    }
}

static void trPrintUsage(const std::string& name)
{
    std::cout
        << "Usage:\n"
        << "  " << name << "                                       one-shot (default)\n"
        << "  " << name << " one-shot                              run all tests once\n"
        << "  " << name << " monitoring [interval_ms]              run indefinitely (Ctrl+C stops)\n"
        << "  " << name << " long-term  <interval_ms> <duration>   run for fixed duration\n"
        << "  " << name << " monitor-test <interval_ms> <duration> long-term + Pi5 hw monitors\n"
        << "  " << name << " event <poll_ms> <criteria> [max_dur]  fire callback on criteria\n"
        << "\nDuration: 500ms  30s  5m  2h  1d  (plain integer = ms)\n"
        << "Criteria: any-fail | all-fail | all-pass | fail-rate-above-N\n";
}

// ---------------------------------------------------------------------------
// Wait for a background test to finish (or Ctrl+C)
// ---------------------------------------------------------------------------

template<typename T>
static void trWaitForCompletion(T& tester)
{
    while (!g_test_stop_flag && tester.isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    tester.stop();
    g_test_stop_flag = false;
}

// ---------------------------------------------------------------------------
// Main template entry point — instantiated once per test binary
// ---------------------------------------------------------------------------

template<typename T>
int runTestApp(const std::string& appName, int argc, char* argv[])
{
    std::string mode_str = (argc > 1) ? argv[1] : "one-shot";

    if (mode_str == "--help" || mode_str == "-h") {
        trPrintUsage(appName);
        return 0;
    }

    // ---- ONE SHOT ----------------------------------------------------------
    if (mode_str == "one-shot" || mode_str == "SINGLE_SHOT") {
        T tester;
        std::cout << appName << " [one-shot]\n";
        tester.runTests(TestMode::SINGLE_SHOT,
                        [](const std::vector<TestResult>& r) { trPrintTable(r); });
        std::cout << appName << " completed.\n";
        return 0;
    }

    // ---- MONITORING (indefinite) -------------------------------------------
    if (mode_str == "monitoring") {
        int interval_ms = 1000;
        if (argc > 2) {
            try { interval_ms = std::stoi(argv[2]); }
            catch (...) { std::cerr << "[ERROR] Invalid interval_ms: " << argv[2] << "\n"; return 1; }
        }
        T tester;
        int cycle = 0;
        std::cout << appName << " [monitoring] interval=" << interval_ms << "ms"
                  << "  Ctrl+C to stop\n";
        std::signal(SIGINT, testRunnerSigHandler);
        tester.runTests(TestMode::MONITORING,
                        [&cycle](const std::vector<TestResult>& r) {
                            trPrintCycleHeader(++cycle, "monitoring", true);
                            trPrintTable(r);
                        },
                        interval_ms);
        trWaitForCompletion(tester);
        std::cout << "\n" << appName << " monitoring stopped.\n";
        return 0;
    }

    // ---- LONG TERM ---------------------------------------------------------
    if (mode_str == "long-term") {
        if (argc < 4) {
            std::cerr << "Usage: " << appName << " long-term <interval_ms> <duration>\n";
            return 1;
        }
        int interval_ms = 0;
        try { interval_ms = std::stoi(argv[2]); }
        catch (...) { std::cerr << "[ERROR] Invalid interval_ms: " << argv[2] << "\n"; return 1; }
        long long duration_ms  = trParseDuration(argv[3]);
        if (duration_ms < 0) return 1;
        T tester;
        std::cout << appName << " [long-term] interval=" << interval_ms
                  << "ms  duration=" << duration_ms << "ms"
                  << "  Ctrl+C to abort early\n";
        std::signal(SIGINT, testRunnerSigHandler);
        tester.runLongTerm(interval_ms, duration_ms,
                           [](const std::vector<TestResult>& r, int cycle) {
                               trPrintCycleHeader(cycle, "long-term", false);
                               trPrintTable(r);
                           });
        trWaitForCompletion(tester);
        std::cout << "\n" << appName << " long-term completed.\n";
        return 0;
    }

    // ---- MONITORING TEST ---------------------------------------------------
    if (mode_str == "monitor-test") {
        if (argc < 4) {
            std::cerr << "Usage: " << appName << " monitor-test <interval_ms> <duration>\n";
            return 1;
        }
        int interval_ms = 0;
        try { interval_ms = std::stoi(argv[2]); }
        catch (...) { std::cerr << "[ERROR] Invalid interval_ms: " << argv[2] << "\n"; return 1; }
        long long duration_ms  = trParseDuration(argv[3]);
        if (duration_ms < 0) return 1;
        T tester;
        std::cout << appName << " [monitor-test] interval=" << interval_ms
                  << "ms  duration=" << duration_ms << "ms"
                  << "  Ctrl+C to abort early\n";
        std::signal(SIGINT, testRunnerSigHandler);
        tester.runMonitoringTest(interval_ms, duration_ms, trPi5MonitorItems(),
                                  [](const MonitoringSnapshot& snap) {
                                      trPrintCycleHeader(snap.cycle, "monitor-test", false);
                                      trPrintMonitorData(snap.monitorData);
                                      trPrintTable(snap.testResults);
                                  });
        trWaitForCompletion(tester);
        std::cout << "\n" << appName << " monitor-test completed.\n";
        return 0;
    }

    // ---- EVENT TEST --------------------------------------------------------
    if (mode_str == "event") {
        if (argc < 4) {
            std::cerr << "Usage: " << appName << " event <poll_ms> <criteria> [max_duration]\n";
            trPrintUsage(appName);
            return 1;
        }
        int poll_ms = 0;
        try { poll_ms = std::stoi(argv[2]); }
        catch (...) { std::cerr << "[ERROR] Invalid poll_ms: " << argv[2] << "\n"; return 1; }
        EventCriteria criteria   = trMakeCriteria(argv[3]);
        long long max_dur_ms = 0;
        if (argc > 4) {
            max_dur_ms = trParseDuration(argv[4]);
            if (max_dur_ms < 0) return 1;
        }
        T tester;
        int event_count = 0;
        std::cout << appName << " [event] poll=" << poll_ms << "ms"
                  << "  criteria='" << criteria.description << "'"
                  << (max_dur_ms > 0 ? "  max=" + std::to_string(max_dur_ms) + "ms" : "  no timeout")
                  << "  Ctrl+C to abort\n";
        std::signal(SIGINT, testRunnerSigHandler);
        tester.runEventTest(poll_ms, criteria,
                            [&event_count](const std::vector<TestResult>& r) {
                                ++event_count;
                                std::cout << "\n*** EVENT FIRED #" << event_count << " ***\n";
                                trPrintTable(r);
                            },
                            max_dur_ms);
        trWaitForCompletion(tester);
        std::cout << "\n" << appName << " event test completed."
                  << "  Events fired: " << event_count << "\n";
        return 0;
    }

    // ---- FALLBACK: unknown argument → one-shot with warning ----------------
    std::cerr << "[WARNING] Unknown mode '" << mode_str << "'. Running one-shot.\n";
    trPrintUsage(appName);
    T tester;
    tester.runTests(TestMode::SINGLE_SHOT,
                    [](const std::vector<TestResult>& r) { trPrintTable(r); });
    return 0;
}
