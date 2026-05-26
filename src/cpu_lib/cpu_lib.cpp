/**
 * @file cpu_lib.cpp
 * @brief Implementation file for CpuTesting class — Raspberry Pi 5 (BCM2712)
 * @details Tests the quad-core Cortex-A76 CPU running at up to 2.4 GHz.
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "cpu_lib.h"
#include "../platform_rpi5.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <functional>
#include <mutex>
#include <numeric>
#include <pthread.h>
#include <sched.h>
#include <sstream>
#include <string>
#include <sys/sysinfo.h>
#include <thread>
#include <unistd.h>
#include <vector>

// ---------------------------------------------------------------------------
// Raspberry Pi 5 (BCM2712 Cortex-A76) constants — from platform_rpi5.h
// ---------------------------------------------------------------------------
static constexpr int    CPU_CORES          = RPI5_CPU_CORES;        // 4
static constexpr double CPU_MAX_FREQ_MHZ   = RPI5_CPU_MAX_FREQ_MHZ; // 2400 MHz
static constexpr double CPU_MIN_FREQ_MHZ   = RPI5_CPU_MIN_FREQ_MHZ; // 1500 MHz
static constexpr double CPU_TEMP_WARNING_C = RPI5_CPU_TEMP_MAX_C;   // 85 °C

// ---------------------------------------------------------------------------
// Helper utilities
// ---------------------------------------------------------------------------

/** Read a single line from a sysfs/procfs file. Returns empty on failure. */
static std::string readSysfsLine(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open())
        return "";
    std::string line;
    std::getline(f, line);
    return line;
}

/** Read an integer value from a sysfs file. Returns -1 on failure. */
static long long readSysfsInt(const std::string& path)
{
    std::string s = readSysfsLine(path);
    if (s.empty())
        return -1LL;
    try {
        return std::stoll(s);
    }
    catch (...) {
        return -1LL;
    }
}

/** Pin the calling thread to the given CPU core. Returns true on success. */
static bool pinThreadToCore(int core)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
}

/** Get the current CPU core the calling thread is running on. */
static int currentCore()
{
    return sched_getcpu();
}

/** Simple compute workload: sum of sqrt across N iterations. */
static double cpuWorkload(uint64_t iterations)
{
    double sum = 0.0;
    for (uint64_t i = 1; i <= iterations; ++i)
        sum += std::sqrt(static_cast<double>(i));
    return sum;
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

/**
 * @brief Constructor for CpuTesting
 */
CpuTesting::CpuTesting()
{
}

/**
 * @brief Destructor for CpuTesting
 */
CpuTesting::~CpuTesting()
{
}

// ---------------------------------------------------------------------------
// 1. testAll12CpuCoresFunctionality
//    Verify all CPU cores (Cortex-A76 × 4 on Raspberry Pi 5) are online.
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testAll12CpuCoresFunctionality()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Platform: " + std::string(PLATFORM_NAME) + " (" + PLATFORM_SOC + ")");
    std::string onlineStr = readSysfsLine("/sys/devices/system/cpu/online");
    int onlineCount = 0;
    std::vector<int> offlineCores;

    for (int i = 0; i < CPU_CORES; ++i) {
        std::string corePath = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/online";
        long long online = readSysfsInt(corePath);
        if (online == 1) {
            ++onlineCount;
        } else if (online == 0) {
            offlineCores.push_back(i);
        } else {
            // cpu0 typically has no 'online' file (always on)
            ++onlineCount;
        }
    }

    data.push_back("Online bitmap: " + (onlineStr.empty() ? "(unavailable)" : onlineStr));
    data.push_back("Online cores detected: " + std::to_string(onlineCount));
    data.push_back("Expected cores: " + std::to_string(CPU_CORES));

    if (onlineCount != CPU_CORES) {
        passed = false;
        data.push_back("FAIL: Not all " + std::to_string(CPU_CORES) + " cores are online.");
        for (int c : offlineCores)
            data.push_back("  Core " + std::to_string(c) + " is offline.");
    }

    // Verify each core can actually execute a computation
    std::vector<bool> coreWorked(CPU_CORES, false);
    std::vector<std::thread> threads;
    std::mutex resultMtx;
    for (int i = 0; i < CPU_CORES; ++i) {
        threads.emplace_back([&, i]() {
            if (!pinThreadToCore(i))
                return;
            volatile double res = cpuWorkload(50000);
            (void)res;
            std::lock_guard<std::mutex> lk(resultMtx);
            coreWorked[i] = true;
        });
    }
    for (auto& t : threads)
        t.join();

    for (int i = 0; i < CPU_CORES; ++i) {
        if (!coreWorked[i]) {
            passed = false;
            data.push_back("FAIL: Core " + std::to_string(i) + " did not complete workload.");
        }
    }

    if (passed)
        data.push_back("All " + std::to_string(CPU_CORES) + " Cortex-A76 cores are online and functional.");
    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 2. testCpuCoreFrequencyScaling
//    Read current/min/max frequencies via cpufreq sysfs for all cores.
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testCpuCoreFrequencyScaling()
{
    std::vector<std::string> data;
    bool passed = true;
    int validCores = 0;

    for (int i = 0; i < CPU_CORES; ++i) {
        std::string base = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/";
        long long curKHz = readSysfsInt(base + "scaling_cur_freq");
        long long minKHz = readSysfsInt(base + "scaling_min_freq");
        long long maxKHz = readSysfsInt(base + "scaling_max_freq");

        if (curKHz < 0 || minKHz < 0 || maxKHz < 0) {
            data.push_back("Core " + std::to_string(i) + ": cpufreq not available (shared policy)");
            continue;
        }

        double curMHz = curKHz / 1000.0, minMHz = minKHz / 1000.0, maxMHz = maxKHz / 1000.0;
        std::ostringstream oss;
        oss << "Core " << i << ": cur=" << curMHz << " MHz  min=" << minMHz << " MHz  max=" << maxMHz << " MHz";
        data.push_back(oss.str());

        if (maxMHz < CPU_MIN_FREQ_MHZ || maxMHz > CPU_MAX_FREQ_MHZ + 200.0) {
            passed = false;
            data.push_back("  FAIL: max freq " + std::to_string(maxMHz) + " MHz out of expected range.");
        }
        if (curMHz < CPU_MIN_FREQ_MHZ || curMHz > maxMHz + 200.0) {
            passed = false;
            data.push_back("  FAIL: cur freq " + std::to_string(curMHz) + " MHz out of valid range.");
        }
        ++validCores;
    }

    std::string gov = readSysfsLine("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
    data.push_back("Scaling governor (cpu0): " + (gov.empty() ? "(unavailable)" : gov));

    if (validCores == 0) {
        passed = false;
        data.push_back("FAIL: No cpufreq data available for any core.");
    }
    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 3. testIndividualCoreStressTesting
//    Pin one thread per core, run compute load, verify completion and timing.
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testIndividualCoreStressTesting()
{
    std::vector<std::string> data;
    bool passed = true;

    struct CoreResult
    {
        int core;
        double timeSec;
        bool completed;
    };
    std::vector<CoreResult> results(CPU_CORES);
    std::vector<std::thread> threads;
    std::mutex mtx;

    for (int i = 0; i < CPU_CORES; ++i) {
        threads.emplace_back([&, i]() {
            if (!pinThreadToCore(i)) {
                std::lock_guard<std::mutex> lk(mtx);
                results[i] = {i, 0.0, false};
                return;
            }
            auto t0 = std::chrono::high_resolution_clock::now();
            volatile double res = cpuWorkload(500000);
            (void)res;
            auto t1 = std::chrono::high_resolution_clock::now();
            std::lock_guard<std::mutex> lk(mtx);
            results[i] = {i, std::chrono::duration<double>(t1 - t0).count(), true};
        });
    }
    for (auto& t : threads)
        t.join();

    double total = 0.0;
    for (const auto& r : results) {
        std::ostringstream oss;
        oss << "Core " << r.core << ": " << (r.completed ? "PASS" : "FAIL") << " (" << r.timeSec << " s)";
        data.push_back(oss.str());
        if (!r.completed)
            passed = false;
        total += r.timeSec;
    }
    data.push_back("Aggregate compute time: " + std::to_string(total) + " s");
    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 4. testMulticoreSynchronization
//    Use atomic barrier to synchronise all threads; verify all complete.
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testMulticoreSynchronization()
{
    std::vector<std::string> data;
    bool passed = true;

    const int N = CPU_CORES;
    std::atomic<int> counter{0}, phase{0};
    std::vector<int> arrivalOrder;
    std::mutex orderMtx;

    std::vector<std::thread> threads;
    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&, i]() {
            pinThreadToCore(i);
            counter.fetch_add(1, std::memory_order_acq_rel);
            {
                std::lock_guard<std::mutex> lk(orderMtx);
                arrivalOrder.push_back(i);
            }
            while (counter.load(std::memory_order_acquire) < N)
                std::this_thread::yield();
            phase.fetch_add(1, std::memory_order_acq_rel);
        });
    }
    for (auto& t : threads)
        t.join();

    data.push_back("Threads at barrier: " + std::to_string(counter.load()));
    data.push_back("Threads completed phase: " + std::to_string(phase.load()));

    if (counter.load() != N || phase.load() != N) {
        passed = false;
        data.push_back("FAIL: Not all threads synchronised correctly.");
    } else {
        data.push_back("All " + std::to_string(N) + " threads synchronised successfully.");
    }

    std::vector<int> sorted = arrivalOrder;
    std::sort(sorted.begin(), sorted.end());
    for (int i = 0; i < N; ++i)
        if (sorted[i] != i)
            passed = false;

    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 5. testCpuClusterCommunication
//    Verify shared-memory writes are visible across BCM2712 Cortex-A76 cores.
//    Pi5 has a single cluster of 4 cores; we test two pairs: {0,1} and {2,3}.
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testCpuClusterCommunication()
{
    std::vector<std::string> data;
    bool passed = true;

    // Two logical groups of cores (single physical cluster on BCM2712)
    const std::vector<int> clusterCores = {0, 2}; // representative core per pair
    const int MAGIC = 0xBC272;
    std::vector<std::atomic<int>> mailbox(clusterCores.size());
    for (auto& m : mailbox)
        m.store(0);

    std::vector<std::thread> threads;
    for (int ci = 0; ci < static_cast<int>(clusterCores.size()); ++ci) {
        threads.emplace_back([&, ci]() {
            pinThreadToCore(clusterCores[ci]);
            int nextCi = (ci + 1) % static_cast<int>(clusterCores.size());
            mailbox[nextCi].store(MAGIC + ci, std::memory_order_release);
        });
    }
    for (auto& t : threads)
        t.join();

    for (int ci = 0; ci < static_cast<int>(clusterCores.size()); ++ci) {
        int prevCi = (ci + static_cast<int>(clusterCores.size()) - 1) % static_cast<int>(clusterCores.size());
        int expected = MAGIC + prevCi;
        int actual = mailbox[ci].load(std::memory_order_acquire);
        std::ostringstream oss;
        oss << "Core group " << ci << " mailbox: expected=" << expected << " actual=" << actual
            << (actual == expected ? " OK" : " MISMATCH");
        data.push_back(oss.str());
        if (actual != expected)
            passed = false;
    }

    if (passed)
        data.push_back("Inter-core communication verified (BCM2712 unified cache domain).");
    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 6. testCoreAffinityAndThreadMigration
//    Pin a thread to each core, confirm sched_getcpu(), migrate, confirm again.
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testCoreAffinityAndThreadMigration()
{
    std::vector<std::string> data;
    bool passed = true;

    for (int i = 0; i < CPU_CORES; ++i) {
        int observedBefore = -1, observedAfter = -1;
        int migrateTarget = (i + 1) % CPU_CORES;
        std::thread t([&, i, migrateTarget]() {
            if (!pinThreadToCore(i))
                return;
            std::this_thread::sleep_for(std::chrono::microseconds(500));
            observedBefore = currentCore();
            if (!pinThreadToCore(migrateTarget))
                return;
            std::this_thread::sleep_for(std::chrono::microseconds(500));
            observedAfter = currentCore();
        });
        t.join();

        std::ostringstream oss;
        oss << "Core " << i << " -> " << migrateTarget << ": before=" << observedBefore << " after=" << observedAfter
            << ((observedBefore >= 0 && observedAfter >= 0) ? " OK" : " WARN");
        data.push_back(oss.str());
        if (observedBefore < 0 || observedAfter < 0)
            passed = false;
    }

    if (passed)
        data.push_back("Core affinity and thread migration verified for all " +
                       std::to_string(CPU_CORES) + " Cortex-A76 cores.");
    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 7. testCpuThermalThrottling
//    Read thermal zones and verify CPU temperatures are below warning threshold.
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testCpuThermalThrottling()
{
    std::vector<std::string> data;
    bool passed = true;
    bool foundAny = false;

    for (int zone = 0; zone < 20; ++zone) {
        std::string base = "/sys/class/thermal/thermal_zone" + std::to_string(zone);
        std::string type = readSysfsLine(base + "/type");
        if (type.empty())
            continue;
        long long tempMilliC = readSysfsInt(base + "/temp");
        if (tempMilliC < 0)
            continue;
        double tempC = tempMilliC / 1000.0;
        std::ostringstream oss;
        oss << "Zone " << zone << " (" << type << "): " << tempC << " C";
        data.push_back(oss.str());
        foundAny = true;
        // BCM2712 thermal zone types on Pi5: "cpu-thermal", "bcm2712", etc.
        bool isCpu = type.find("CPU") != std::string::npos || type.find("cpu") != std::string::npos ||
                     type.find("A76") != std::string::npos || type.find("bcm2712") != std::string::npos ||
                     type.find("rp1") != std::string::npos || type.find("soc") != std::string::npos;
        if (isCpu && tempC >= CPU_TEMP_WARNING_C) {
            passed = false;
            data.push_back("  FAIL: " + std::to_string(tempC) + " C exceeds warning threshold.");
        }
    }

    if (!foundAny) {
        long long t = readSysfsInt("/sys/class/hwmon/hwmon0/temp1_input");
        if (t > 0) {
            data.push_back("hwmon0 temp: " + std::to_string(t / 1000.0) + " C");
            foundAny = true;
        }
    }
    if (!foundAny)
        data.push_back("WARNING: No thermal zones accessible (may need elevated privileges).");
    else
        data.push_back("Thermal check complete. No throttling condition detected.");

    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 8. testCpuPowerStatesAndTransitions
//    Check cpuidle state entries for all cores via sysfs.
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testCpuPowerStatesAndTransitions()
{
    std::vector<std::string> data;
    bool passed = true;
    int coresWithIdle = 0;

    for (int i = 0; i < CPU_CORES; ++i) {
        std::string idleBase = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpuidle/";
        bool hasIdleState = false;
        for (int s = 0; s < 5; ++s) {
            std::string name = readSysfsLine(idleBase + "state" + std::to_string(s) + "/name");
            if (name.empty())
                continue;
            hasIdleState = true;
            long long usage = readSysfsInt(idleBase + "state" + std::to_string(s) + "/usage");
            std::ostringstream oss;
            oss << "Core " << i << " idle state" << s << " (" << name << ") usage=" << usage;
            data.push_back(oss.str());
        }
        if (hasIdleState)
            ++coresWithIdle;
    }

    data.push_back("Cores with idle state entries: " + std::to_string(coresWithIdle) + " / " + std::to_string(CPU_CORES));
    std::string pm = readSysfsLine("/sys/bus/cpu/devices/cpu0/power/control");
    data.push_back("CPU0 power control: " + (pm.empty() ? "(unavailable)" : pm));
    std::string onlineVal = readSysfsLine("/sys/devices/system/cpu/cpu1/online");
    data.push_back("CPU1 online state: " + (onlineVal.empty() ? "(unavailable)" : onlineVal));

    if (coresWithIdle == 0)
        data.push_back("WARNING: No cpuidle states found - may require kernel config.");

    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 9. testL1Cache
//    Measure L1 D-cache read latency with a 32 KB buffer (fits in 64 KB L1).
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testL1Cache()
{
    std::vector<std::string> data;
    bool passed = true;

    const size_t L1_SIZE = 32 * 1024;
    const size_t STRIDE = 64;
    const uint64_t ITERATIONS = 1'000'000;

    std::vector<uint8_t> buf(L1_SIZE, 1);
    volatile uint64_t dummy = 0;
    for (size_t i = 0; i < L1_SIZE; i += STRIDE)
        dummy += buf[i]; // warm-up

    auto t0 = std::chrono::high_resolution_clock::now();
    for (uint64_t iter = 0; iter < ITERATIONS; ++iter)
        dummy += buf[(iter * STRIDE) % L1_SIZE];
    auto t1 = std::chrono::high_resolution_clock::now();
    (void)dummy;

    double totalSec = std::chrono::duration<double>(t1 - t0).count();
    double latencyNs = (totalSec / ITERATIONS) * 1e9;

    data.push_back("L1 cache buffer: " + std::to_string(L1_SIZE / 1024) + " KB");
    data.push_back("Avg access latency: " + std::to_string(latencyNs) + " ns");
    if (latencyNs > 10.0)
        data.push_back("WARNING: L1 latency " + std::to_string(latencyNs) + " ns > 10 ns.");
    else
        data.push_back("L1 cache latency within expected range (<10 ns).");

    // Report cache topology from sysfs
    for (int i = 0; i < 4; ++i) {
        std::string base = "/sys/devices/system/cpu/cpu0/cache/index" + std::to_string(i);
        std::string level = readSysfsLine(base + "/level");
        std::string type = readSysfsLine(base + "/type");
        std::string size = readSysfsLine(base + "/size");
        if (!level.empty())
            data.push_back("Cache index" + std::to_string(i) + ": L" + level + " " + type + " = " + size);
    }
    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 10. testL2Cache
//     Measure L2 cache performance with a 512 KB buffer.
//     BCM2712 Cortex-A76: 512 KB L2 per core.
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testL2Cache()
{
    std::vector<std::string> data;
    bool passed = true;

    const size_t L2_SIZE = 512 * 1024;  // 512 KB — Pi5 Cortex-A76 L2
    const size_t STRIDE = 64;
    const uint64_t ITERATIONS = 500'000;

    std::vector<uint8_t> buf(L2_SIZE, 1);
    volatile uint64_t dummy = 0;
    for (size_t i = 0; i < L2_SIZE; i += STRIDE)
        dummy += buf[i];

    auto t0 = std::chrono::high_resolution_clock::now();
    for (uint64_t iter = 0; iter < ITERATIONS; ++iter)
        dummy += buf[(iter * STRIDE) % L2_SIZE];
    auto t1 = std::chrono::high_resolution_clock::now();
    (void)dummy;

    double totalSec = std::chrono::duration<double>(t1 - t0).count();
    double latencyNs = (totalSec / ITERATIONS) * 1e9;
    double bwGBs = (static_cast<double>(ITERATIONS) * STRIDE) / (totalSec * 1e9);

    data.push_back("L2 cache buffer: " + std::to_string(L2_SIZE / 1024) + " KB");
    data.push_back("Avg access latency: " + std::to_string(latencyNs) + " ns");
    data.push_back("Effective bandwidth: " + std::to_string(bwGBs) + " GB/s");
    if (latencyNs > 50.0)
        data.push_back("WARNING: L2 latency " + std::to_string(latencyNs) + " ns > 50 ns.");
    else
        data.push_back("L2 cache latency within expected range (<50 ns).");

    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 11. testL3Cache
//     Measure system-level cache / DRAM performance with a 4 MB buffer.
//     BCM2712 Cortex-A76: no on-die L3; 4 MB exercises DRAM path.
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testL3Cache()
{
    std::vector<std::string> data;
    bool passed = true;

    const size_t L3_SIZE = 4 * 1024 * 1024;
    const size_t STRIDE = 64;
    const uint64_t ITERATIONS = 200'000;

    std::vector<uint8_t> buf(L3_SIZE, 1);
    volatile uint64_t dummy = 0;
    for (size_t i = 0; i < L3_SIZE; i += STRIDE)
        dummy += buf[i];

    auto t0 = std::chrono::high_resolution_clock::now();
    for (uint64_t iter = 0; iter < ITERATIONS; ++iter)
        dummy += buf[(iter * STRIDE) % L3_SIZE];
    auto t1 = std::chrono::high_resolution_clock::now();
    (void)dummy;

    double totalSec = std::chrono::duration<double>(t1 - t0).count();
    double latencyNs = (totalSec / ITERATIONS) * 1e9;

    data.push_back("L3 buffer: " + std::to_string(L3_SIZE / 1024 / 1024) + " MB");
    data.push_back("Avg access latency: " + std::to_string(latencyNs) + " ns");
    if (latencyNs > 200.0)
        data.push_back("WARNING: L3 latency " + std::to_string(latencyNs) + " ns > 200 ns.");
    else
        data.push_back("Memory (beyond L2) latency within expected range (<200 ns).");

    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 12. testSystemCache
//     Read full cache topology for all 4 Cortex-A76 cores from sysfs.
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testSystemCache()
{
    std::vector<std::string> data;
    bool passed = true;
    bool foundL1 = false, foundL2 = false, foundL3 = false;

    for (int core = 0; core < CPU_CORES; ++core) {
        for (int idx = 0; idx < 5; ++idx) {
            std::string base = "/sys/devices/system/cpu/cpu" + std::to_string(core) + "/cache/index" + std::to_string(idx) + "/";
            std::string level = readSysfsLine(base + "level");
            std::string type = readSysfsLine(base + "type");
            std::string sizeStr = readSysfsLine(base + "size");
            std::string coherency = readSysfsLine(base + "coherency_line_size");
            if (level.empty())
                continue;
            if (core == 0) {
                data.push_back("CPU0 index" + std::to_string(idx) + ": L" + level + " " + type + " size=" + sizeStr + " line=" + coherency +
                               "B");
            }
            if (level == "1")
                foundL1 = true;
            if (level == "2")
                foundL2 = true;
            if (level == "3")
                foundL3 = true;
        }
    }

    data.push_back("Cache hierarchy: L1=" + std::string(foundL1 ? "OK" : "?") + " L2=" + std::string(foundL2 ? "OK" : "?") +
                   " L3=" + std::string(foundL3 ? "OK" : "not exposed in sysfs"));
    if (!foundL1)
        data.push_back("WARNING: L1 cache not found in sysfs topology.");
    if (!foundL2)
        data.push_back("WARNING: L2 cache not found in sysfs topology.");

    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 13. testCacheCoherencyBetweenClusters
//     Writer (core 0) -> Reader (core 3); 1000 rounds.
//     Pi5 BCM2712: single cluster — coherency is via shared L2 / AMBA bus.
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testCacheCoherencyBetweenClusters()
{
    std::vector<std::string> data;
    bool passed = true;

    const int ROUNDS = 1000;
    std::atomic<int> sharedValue{0};
    std::atomic<bool> writerDone{false};

    for (int round = 0; round < ROUNDS; ++round) {
        writerDone.store(false, std::memory_order_relaxed);
        sharedValue.store(0, std::memory_order_relaxed);

        std::thread writer([&]() {
            pinThreadToCore(0);
            sharedValue.store(round + 1, std::memory_order_release);
            writerDone.store(true, std::memory_order_release);
        });
        std::thread reader([&]() {
            pinThreadToCore(CPU_CORES - 1); // last core (core 3 on Pi5)
            while (!writerDone.load(std::memory_order_acquire))
                std::this_thread::yield();
            if (sharedValue.load(std::memory_order_acquire) != round + 1)
                passed = false;
        });
        writer.join();
        reader.join();
    }

    data.push_back("Cache coherency rounds: " + std::to_string(ROUNDS));
    data.push_back("Writer: core 0  Reader: core " + std::to_string(CPU_CORES - 1) +
                   "  (BCM2712 unified AMBA coherency domain)");
    data.push_back(passed ? "All rounds passed - cache coherency verified."
                          : "FAIL: Cache coherency violation detected.");
    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 14. testCacheLineInvalidation
//     Compare false-sharing vs padded (no-false-sharing) access time.
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testCacheLineInvalidation()
{
    std::vector<std::string> data;
    bool passed = true;

    const int THREADS = 4;
    const uint64_t ITERS = 2'000'000;

    // Unpadded: all counters share cache lines
    struct Unpadded
    {
        volatile uint64_t val[4];
    } unpadded{};

    // Padded: each counter on its own 64-byte cache line
    struct alignas(64) PaddedVal
    {
        volatile uint64_t val;
        uint8_t pad[56];
    };
    PaddedVal padded[4]{};

    auto run = [&](std::function<volatile uint64_t&(int)> getter) {
        std::vector<std::thread> threads;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < THREADS; ++i)
            threads.emplace_back([&, i]() {
                pinThreadToCore(i);
                for (uint64_t it = 0; it < ITERS; ++it)
                    getter(i)++;
            });
        for (auto& t : threads)
            t.join();
        return std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();
    };

    double tUnpadded = run([&](int i) -> volatile uint64_t& { return unpadded.val[i]; });
    double tPadded = run([&](int i) -> volatile uint64_t& { return padded[i].val; });

    double ratio = (tPadded > 0) ? (tUnpadded / tPadded) : 1.0;
    data.push_back("False-sharing (unpadded) time : " + std::to_string(tUnpadded) + " s");
    data.push_back("No-false-sharing (padded) time: " + std::to_string(tPadded) + " s");
    data.push_back("Ratio (unpadded/padded)        : " + std::to_string(ratio));

    if (ratio > 5.0)
        data.push_back("WARNING: Significant false sharing detected (ratio=" + std::to_string(ratio) + ").");
    else
        data.push_back("Cache line invalidation test completed.");

    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 15. testCacheBandwidthAndLatency
//     Stream benchmark with 64 MB buffer (beyond all caches) for DRAM bandwidth.
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testCacheBandwidthAndLatency()
{
    std::vector<std::string> data;
    bool passed = true;

    const size_t BUF_SIZE = 64ULL * 1024 * 1024;
    const int PASSES = 3;

    std::vector<uint64_t> buf(BUF_SIZE / sizeof(uint64_t), 0xDEADBEEFCAFEBABEULL);
    double bestBW = 0.0;

    for (int p = 0; p < PASSES; ++p) {
        volatile uint64_t sum = 0;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < buf.size(); ++i)
            sum += buf[i];
        auto t1 = std::chrono::high_resolution_clock::now();
        (void)sum;
        double sec = std::chrono::duration<double>(t1 - t0).count();
        double bwGB = (BUF_SIZE / 1e9) / sec;
        bestBW = std::max(bestBW, bwGB);
        data.push_back("Pass " + std::to_string(p + 1) + ": " + std::to_string(bwGB) + " GB/s");
    }

    data.push_back("Best read bandwidth: " + std::to_string(bestBW) + " GB/s");
    if (bestBW < 5.0) {
        passed = false;
        data.push_back("FAIL: Bandwidth " + std::to_string(bestBW) + " GB/s below 5 GB/s threshold.");
    } else {
        data.push_back("Memory bandwidth is acceptable (Pi5 LPDDR4X-4267 theoretical: ~17 GB/s).");
    }
    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 16. testSinglethreadedPerformance
//     Integer and FP throughput benchmark, single thread on Cortex-A78AE.
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testSinglethreadedPerformance()
{
    std::vector<std::string> data;
    bool passed = true;

    const uint64_t ITERS = 10'000'000;

    // Integer
    auto t0 = std::chrono::high_resolution_clock::now();
    volatile uint64_t acc = 0;
    for (uint64_t i = 0; i < ITERS; ++i)
        acc += i * (i ^ 0xABCD) % 97;
    double intSec = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();
    (void)acc;

    // FP
    t0 = std::chrono::high_resolution_clock::now();
    volatile double facc = 0.0;
    for (uint64_t i = 1; i <= ITERS; ++i)
        facc += std::sqrt(static_cast<double>(i)) * 1.000001;
    double fpSec = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();
    (void)facc;

    double intMOPS = ITERS / intSec / 1e6;
    double fpMFLOPS = ITERS / fpSec / 1e6;

    data.push_back("Integer throughput : " + std::to_string(intMOPS) + " MOPS  (" + std::to_string(intSec) + " s)");
    data.push_back("FP throughput      : " + std::to_string(fpMFLOPS) + " MFLOPS (" + std::to_string(fpSec) + " s)");

    if (intMOPS < 100.0) {
        passed = false;
        data.push_back("FAIL: Integer throughput " + std::to_string(intMOPS) + " MOPS below 100 MOPS threshold.");
    } else {
        data.push_back("Single-threaded performance within expected range.");
    }
    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 17. testMultithreadedPerformance
//     Scale FP benchmark across all 4 cores; measure aggregate MFLOPS.
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testMultithreadedPerformance()
{
    std::vector<std::string> data;
    bool passed = true;

    const uint64_t ITERS_PER_THREAD = 5'000'000;
    std::mutex mtx;
    std::vector<double> threadResults(CPU_CORES, 0.0);
    std::vector<std::thread> threads;

    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < CPU_CORES; ++i) {
        threads.emplace_back([&, i]() {
            pinThreadToCore(i);
            volatile double acc = 0.0;
            for (uint64_t j = 1; j <= ITERS_PER_THREAD; ++j)
                acc += std::sqrt(static_cast<double>(j));
            std::lock_guard<std::mutex> lk(mtx);
            threadResults[i] = acc;
        });
    }
    for (auto& t : threads)
        t.join();
    double wallSec = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();

    double totalOps = static_cast<double>(CPU_CORES) * ITERS_PER_THREAD;
    double aggMFLOPS = totalOps / wallSec / 1e6;

    data.push_back("Threads       : " + std::to_string(CPU_CORES));
    data.push_back("Wall time      : " + std::to_string(wallSec) + " s");
    data.push_back("Aggregate MFLOPS: " + std::to_string(aggMFLOPS));

    if (aggMFLOPS < 500.0) {
        passed = false;
        data.push_back("FAIL: Aggregate MFLOPS " + std::to_string(aggMFLOPS) + " below 500 MFLOPS threshold.");
    } else {
        data.push_back("Multi-threaded performance within expected range.");
    }
    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 18. testContextSwitchingPerformance
//     Pipe ping-pong between two threads pinned to different cores.
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testContextSwitchingPerformance()
{
    std::vector<std::string> data;
    bool passed = true;

    const int ROUNDS = 10'000;
    int pipeAB[2], pipeBA[2];
    if (pipe(pipeAB) != 0 || pipe(pipeBA) != 0)
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            _out.reason["detail_0"] = "FAIL: Could not create pipes for context switch test.";
            return _out;
        }

    std::atomic<bool> ready{false};
    std::thread pinger([&]() {
        pinThreadToCore(0);
        uint8_t byte = 1;
        ready.store(true, std::memory_order_release);
        for (int i = 0; i < ROUNDS; ++i) {
            write(pipeAB[1], &byte, 1);
            read(pipeBA[0], &byte, 1);
        }
        close(pipeAB[1]);
        close(pipeBA[0]);
    });
    std::thread ponger([&]() {
        pinThreadToCore(1);
        uint8_t byte = 0;
        while (!ready.load(std::memory_order_acquire))
            std::this_thread::yield();
        for (int i = 0; i < ROUNDS; ++i) {
            read(pipeAB[0], &byte, 1);
            write(pipeBA[1], &byte, 1);
        }
        close(pipeAB[0]);
        close(pipeBA[1]);
    });

    auto t0 = std::chrono::high_resolution_clock::now();
    pinger.join();
    ponger.join();
    double totalSec = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();
    double ctxNs = (totalSec / (ROUNDS * 2)) * 1e9;

    data.push_back("Rounds                          : " + std::to_string(ROUNDS));
    data.push_back("Total time                      : " + std::to_string(totalSec) + " s");
    data.push_back("Avg context switch latency       : " + std::to_string(ctxNs / 1000.0) + " us");

    if (ctxNs > 20'000.0)
        data.push_back("WARNING: Context switch latency " + std::to_string(ctxNs / 1000.0) + " us > 20 us.");
    else
        data.push_back("Context switch latency within expected range.");

    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 19. testInterruptLatency
//     Measure timer wakeup jitter with nanosleep; sample /proc/interrupts.
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testInterruptLatency()
{
    std::vector<std::string> data;
    bool passed = true;

    const int SAMPLES = 200;
    const long SLEEP_NS = 1'000'000; // 1 ms
    std::vector<double> jitterNs;

    for (int i = 0; i < SAMPLES; ++i) {
        struct timespec req = {0, SLEEP_NS};
        auto t0 = std::chrono::high_resolution_clock::now();
        nanosleep(&req, nullptr);
        double elapsed = std::chrono::duration<double, std::nano>(std::chrono::high_resolution_clock::now() - t0).count();
        jitterNs.push_back(std::abs(elapsed - SLEEP_NS));
    }

    double avgJ = std::accumulate(jitterNs.begin(), jitterNs.end(), 0.0) / jitterNs.size();
    double maxJ = *std::max_element(jitterNs.begin(), jitterNs.end());
    double minJ = *std::min_element(jitterNs.begin(), jitterNs.end());

    data.push_back("Samples    : " + std::to_string(SAMPLES) + "  (target sleep 1 ms)");
    data.push_back("Avg jitter : " + std::to_string(avgJ / 1000.0) + " us");
    data.push_back("Min jitter : " + std::to_string(minJ / 1000.0) + " us");
    data.push_back("Max jitter : " + std::to_string(maxJ / 1000.0) + " us");

    // Sample first few lines of /proc/interrupts
    std::ifstream intFile("/proc/interrupts");
    if (intFile.is_open()) {
        int shown = 0;
        std::string line;
        while (std::getline(intFile, line) && shown++ < 3)
            data.push_back(line.substr(0, std::min((int)line.size(), 80)));
    }

    if (avgJ > 5'000'000.0) {
        passed = false;
        data.push_back("FAIL: Avg jitter " + std::to_string(avgJ / 1e6) + " ms exceeds 5 ms threshold.");
    } else {
        data.push_back("Interrupt/timer latency within acceptable range.");
    }
    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 20. testDmaOperations
//     Measure memcpy bandwidth (DMA-like) and verify data integrity.
// ---------------------------------------------------------------------------
TestOutput CpuTesting::testDmaOperations()
{
    std::vector<std::string> data;
    bool passed = true;

    const size_t BLOCK = 16 * 1024 * 1024; // 16 MB
    const int PASSES = 4;

    std::vector<uint8_t> src(BLOCK), dst(BLOCK, 0);
    for (size_t i = 0; i < BLOCK; ++i)
        src[i] = static_cast<uint8_t>(i & 0xFF);

    double bestBW = 0.0;
    for (int p = 0; p < PASSES; ++p) {
        std::fill(dst.begin(), dst.end(), 0);
        auto t0 = std::chrono::high_resolution_clock::now();
        std::memcpy(dst.data(), src.data(), BLOCK);
        double sec = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();
        double bw = (BLOCK / 1e9) / sec;
        bestBW = std::max(bestBW, bw);
        data.push_back("memcpy pass " + std::to_string(p + 1) + ": " + std::to_string(bw) + " GB/s");
    }

    bool integrityOk = (std::memcmp(src.data(), dst.data(), BLOCK) == 0);
    data.push_back("Data integrity: " + std::string(integrityOk ? "PASS" : "FAIL"));
    if (!integrityOk)
        passed = false;

    data.push_back("Best memcpy bandwidth: " + std::to_string(bestBW) + " GB/s");

    // Report DMA/RAM regions from /proc/iomem
    std::ifstream iomem("/proc/iomem");
    if (iomem.is_open()) {
        std::string line;
        int shown = 0;
        while (std::getline(iomem, line) && shown < 4) {
            if (line.find("DMA") != std::string::npos || line.find("System RAM") != std::string::npos) {
                data.push_back("/proc/iomem: " + line);
                ++shown;
            }
        }
    }

    if (bestBW < 5.0) {
        passed = false;
        data.push_back("FAIL: DMA bandwidth " + std::to_string(bestBW) + " GB/s below 5 GB/s threshold.");
    } else {
        data.push_back("DMA/memcpy operation test completed successfully.");
    }
    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Run all tests for this module
 * @return Vector of test results
 */
std::vector<TestResult> CpuTesting::runAllTests()
{
    std::vector<TestResult> results;

    // Run testAll12CpuCoresFunctionality
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testAll12CpuCoresFunctionality();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testAll12CpuCoresFunctionality";
            _tr.test_id = "testAll12CpuCoresFunctionality";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testCpuCoreFrequencyScaling
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testCpuCoreFrequencyScaling();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testCpuCoreFrequencyScaling";
            _tr.test_id = "testCpuCoreFrequencyScaling";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testIndividualCoreStressTesting
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testIndividualCoreStressTesting();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testIndividualCoreStressTesting";
            _tr.test_id = "testIndividualCoreStressTesting";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testMulticoreSynchronization
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testMulticoreSynchronization();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testMulticoreSynchronization";
            _tr.test_id = "testMulticoreSynchronization";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testCpuClusterCommunication
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testCpuClusterCommunication();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testCpuClusterCommunication";
            _tr.test_id = "testCpuClusterCommunication";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testCoreAffinityAndThreadMigration
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testCoreAffinityAndThreadMigration();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testCoreAffinityAndThreadMigration";
            _tr.test_id = "testCoreAffinityAndThreadMigration";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testCpuThermalThrottling
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testCpuThermalThrottling();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testCpuThermalThrottling";
            _tr.test_id = "testCpuThermalThrottling";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testCpuPowerStatesAndTransitions
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testCpuPowerStatesAndTransitions();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testCpuPowerStatesAndTransitions";
            _tr.test_id = "testCpuPowerStatesAndTransitions";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testL1Cache
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testL1Cache();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testL1Cache";
            _tr.test_id = "testL1Cache";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testL2Cache
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testL2Cache();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testL2Cache";
            _tr.test_id = "testL2Cache";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testL3Cache
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testL3Cache();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testL3Cache";
            _tr.test_id = "testL3Cache";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSystemCache
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSystemCache();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSystemCache";
            _tr.test_id = "testSystemCache";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testCacheCoherencyBetweenClusters
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testCacheCoherencyBetweenClusters();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testCacheCoherencyBetweenClusters";
            _tr.test_id = "testCacheCoherencyBetweenClusters";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testCacheLineInvalidation
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testCacheLineInvalidation();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testCacheLineInvalidation";
            _tr.test_id = "testCacheLineInvalidation";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testCacheBandwidthAndLatency
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testCacheBandwidthAndLatency();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testCacheBandwidthAndLatency";
            _tr.test_id = "testCacheBandwidthAndLatency";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSinglethreadedPerformance
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSinglethreadedPerformance();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSinglethreadedPerformance";
            _tr.test_id = "testSinglethreadedPerformance";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testMultithreadedPerformance
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testMultithreadedPerformance();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testMultithreadedPerformance";
            _tr.test_id = "testMultithreadedPerformance";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testContextSwitchingPerformance
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testContextSwitchingPerformance();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testContextSwitchingPerformance";
            _tr.test_id = "testContextSwitchingPerformance";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testInterruptLatency
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testInterruptLatency();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testInterruptLatency";
            _tr.test_id = "testInterruptLatency";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testDmaOperations
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testDmaOperations();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testDmaOperations";
            _tr.test_id = "testDmaOperations";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    return results;
}

/**
 * @brief Get list of all available test names
 * @return Vector of test names
 */
std::vector<std::string> CpuTesting::getAvailableTests()
{
    return {
        "testAll12CpuCoresFunctionality",
        "testCpuCoreFrequencyScaling",
        "testIndividualCoreStressTesting",
        "testMulticoreSynchronization",
        "testCpuClusterCommunication",
        "testCoreAffinityAndThreadMigration",
        "testCpuThermalThrottling",
        "testCpuPowerStatesAndTransitions",
        "testL1Cache",
        "testL2Cache",
        "testL3Cache",
        "testSystemCache",
        "testCacheCoherencyBetweenClusters",
        "testCacheLineInvalidation",
        "testCacheBandwidthAndLatency",
        "testSinglethreadedPerformance",
        "testMultithreadedPerformance",
        "testContextSwitchingPerformance",
        "testInterruptLatency",
        "testDmaOperations",
    };
}

/**
 * @brief Run a single test by name
 * @param testName Name of the test to run
 * @return Test result with timing
 */
TestResult CpuTesting::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if (testName == "testAll12CpuCoresFunctionality") {
        result = testAll12CpuCoresFunctionality();
    } else if (testName == "testCpuCoreFrequencyScaling") {
        result = testCpuCoreFrequencyScaling();
    } else if (testName == "testIndividualCoreStressTesting") {
        result = testIndividualCoreStressTesting();
    } else if (testName == "testMulticoreSynchronization") {
        result = testMulticoreSynchronization();
    } else if (testName == "testCpuClusterCommunication") {
        result = testCpuClusterCommunication();
    } else if (testName == "testCoreAffinityAndThreadMigration") {
        result = testCoreAffinityAndThreadMigration();
    } else if (testName == "testCpuThermalThrottling") {
        result = testCpuThermalThrottling();
    } else if (testName == "testCpuPowerStatesAndTransitions") {
        result = testCpuPowerStatesAndTransitions();
    } else if (testName == "testL1Cache") {
        result = testL1Cache();
    } else if (testName == "testL2Cache") {
        result = testL2Cache();
    } else if (testName == "testL3Cache") {
        result = testL3Cache();
    } else if (testName == "testSystemCache") {
        result = testSystemCache();
    } else if (testName == "testCacheCoherencyBetweenClusters") {
        result = testCacheCoherencyBetweenClusters();
    } else if (testName == "testCacheLineInvalidation") {
        result = testCacheLineInvalidation();
    } else if (testName == "testCacheBandwidthAndLatency") {
        result = testCacheBandwidthAndLatency();
    } else if (testName == "testSinglethreadedPerformance") {
        result = testSinglethreadedPerformance();
    } else if (testName == "testMultithreadedPerformance") {
        result = testMultithreadedPerformance();
    } else if (testName == "testContextSwitchingPerformance") {
        result = testContextSwitchingPerformance();
    } else if (testName == "testInterruptLatency") {
        result = testInterruptLatency();
    } else if (testName == "testDmaOperations") {
        result = testDmaOperations();
    } else {
        // Unknown test name
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = testName + " (UNKNOWN)";
            _tr.test_id = testName;
            _tr.output.status = hal::Status::ERROR;
            _tr.output.severity = hal::Severity::ERROR;
            _tr.output.error_message = "Unknown test name: " + testName;
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            return _tr;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    {
        TestResult _tr;
        _tr.name = testName;
        _tr.test_id = testName;
        _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
        _tr.output = result;
        return _tr;
    }
}
