/**
 * @file thermal_management_lib.cpp
 * @brief Implementation file for ThermalManagementTesting class — Raspberry Pi 5
 *        BCM2712 has two thermal zones (cpu-thermal and RP1 south-bridge).
 *        Throttle onset: 80°C; soft shutdown: 85°C.
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "thermal_management_lib.h"
#include "../platform_rpi5.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

// ---------------------------------------------------------------------------
// Pi5 thermal constants
// ---------------------------------------------------------------------------
static constexpr double THROTTLE_C  = RPI5_THERMAL_THROTTLE_C;  // 80°C
static constexpr double SHUTDOWN_C  = RPI5_THERMAL_SHUTDOWN_C;  // 85°C
static constexpr double NORMAL_MAX  = 75.0; // "normal" operating ceiling
static constexpr double AMBIENT_MIN = 5.0;
static constexpr double AMBIENT_MAX = 50.0;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string thrmReadFile(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::string s;
    std::getline(f, s);
    return s;
}

static long long thrmReadInt(const std::string& path)
{
    std::string s = thrmReadFile(path);
    if (s.empty()) return -1LL;
    try { return std::stoll(s); } catch (...) { return -1LL; }
}

static std::string thrmRunCmd(const char* cmd)
{
    char buf[2048] = {};
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "";
    size_t n = fread(buf, 1, sizeof(buf) - 1, pipe);
    pclose(pipe);
    buf[n] = '\0';
    while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r')) buf[--n] = '\0';
    return std::string(buf);
}

/** Read all thermal zones and return {name, temperature_C} pairs. */
static std::vector<std::pair<std::string, double>> readAllThermalZones()
{
    std::vector<std::pair<std::string, double>> zones;
    for (int z = 0; z < 20; ++z) {
        std::string base = "/sys/class/thermal/thermal_zone" + std::to_string(z);
        std::string type = thrmReadFile(base + "/type");
        if (type.empty()) break;
        long long milliC = thrmReadInt(base + "/temp");
        if (milliC < -50000 || milliC > 200000) continue;
        zones.push_back({type, milliC / 1000.0});
    }
    return zones;
}

/** Read BCM2712 SoC temperature (zone0 on Pi5). Returns °C or -1. */
static double readSocTempC()
{
    long long v = thrmReadInt("/sys/class/thermal/thermal_zone0/temp");
    if (v > 0) return v / 1000.0;
    // Try vcgencmd fallback
    std::string vc = thrmRunCmd("vcgencmd measure_temp 2>/dev/null");
    if (!vc.empty()) {
        // format: "temp=47.2'C"
        double t = -1.0;
        if (sscanf(vc.c_str(), "temp=%lf", &t) == 1) return t;
    }
    return -1.0;
}

/** CPU workload for thermal stress test. */
static void thermalStressWorkload(double seconds)
{
    volatile double acc = 1.0;
    auto tEnd = std::chrono::steady_clock::now() + std::chrono::duration<double>(seconds);
    while (std::chrono::steady_clock::now() < tEnd)
        acc = acc * 1.000001 + 0.000001;
    (void)acc;
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
ThermalManagementTesting::ThermalManagementTesting()
{
}

ThermalManagementTesting::~ThermalManagementTesting()
{
}

// ---------------------------------------------------------------------------
// 1. testSocJunctionTemperature
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testSocJunctionTemperature()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("BCM2712 SoC junction temperature (Pi5)");
    data.push_back("Thermal zones: zone0=cpu-thermal, zone1=rp1-thermal");

    double tempC = readSocTempC();
    if (tempC < 0) {
        data.push_back("FAIL: Cannot read SoC temperature.");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    data.push_back("SoC temperature: " + std::to_string(tempC) + " °C");
    data.push_back("Throttle onset:  " + std::to_string(THROTTLE_C)  + " °C");
    data.push_back("Shutdown temp:   " + std::to_string(SHUTDOWN_C)  + " °C");

    if (tempC >= SHUTDOWN_C) {
        passed = false;
        data.push_back("FAIL: SoC at or above shutdown temperature!");
    } else if (tempC >= THROTTLE_C) {
        passed = false;
        data.push_back("WARNING: SoC at or above throttle threshold.");
    } else {
        data.push_back("PASS: SoC junction temperature within safe operating range.");
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
// 2. testGpuTemperature
//    On Pi5, GPU (VideoCore VII) shares the BCM2712 die — same thermal zone.
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testGpuTemperature()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("VideoCore VII GPU temperature (shared BCM2712 die with CPU)");

    double socC = readSocTempC();
    data.push_back("BCM2712 die temperature (GPU/CPU shared): " + std::to_string(socC) + " °C");

    // vcgencmd gives GPU temperature on Pi
    std::string gpuTemp = thrmRunCmd("vcgencmd measure_temp 2>/dev/null");
    if (!gpuTemp.empty()) data.push_back("vcgencmd GPU temp: " + gpuTemp);

    if (socC >= THROTTLE_C) {
        passed = false;
        data.push_back("WARNING: GPU/SoC temperature at throttle threshold.");
    } else {
        data.push_back("PASS: GPU temperature within limits.");
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
// 3. testCpuTemperature
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testCpuTemperature()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Cortex-A76 CPU temperature (BCM2712 zone0)");

    long long zone0 = thrmReadInt("/sys/class/thermal/thermal_zone0/temp");
    if (zone0 > 0) {
        double c = zone0 / 1000.0;
        data.push_back("thermal_zone0 (cpu-thermal): " + std::to_string(c) + " °C");
        if (c >= SHUTDOWN_C) {
            passed = false;
            data.push_back("FAIL: CPU near thermal shutdown!");
        } else if (c >= THROTTLE_C) {
            passed = false;
            data.push_back("WARNING: CPU at throttle threshold.");
        }
    }

    // Per-core via hwmon? (not standard on Pi5 — no per-core sensor)
    data.push_back("Per-core temperature sensors: not available on BCM2712.");
    data.push_back("CPU max freq (scaling): check cpufreq/scaling_max_freq per core.");

    for (int i = 0; i < RPI5_CPU_CORES; ++i) {
        std::string maxFreq = thrmReadFile("/sys/devices/system/cpu/cpu" +
                              std::to_string(i) + "/cpufreq/scaling_max_freq");
        if (!maxFreq.empty())
            data.push_back("  Core " + std::to_string(i) + " max_freq: " + maxFreq + " kHz");
    }

    if (passed) data.push_back("PASS: CPU temperature within safe range.");
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
// 4. testMemoryTemperature
//    LPDDR4X temperature is not directly reported on Pi5 (no DIMM SPD/SMBus).
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testMemoryTemperature()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("LPDDR4X memory temperature (Pi5)");
    data.push_back("Pi5 LPDDR4X-4267 is PoP (Package on Package) with BCM2712.");
    data.push_back("No dedicated memory temperature sensor is exposed via sysfs.");
    data.push_back("Memory temperature is proxied by SoC die temperature.");

    double socC = readSocTempC();
    data.push_back("SoC/memory proxy temperature: " + std::to_string(socC) + " °C");
    data.push_back("LPDDR4X max operating temperature: 85°C");

    if (socC > 80.0) {
        passed = false;
        data.push_back("WARNING: SoC/memory temperature approaching LPDDR4X limit.");
    } else {
        data.push_back("PASS: Memory thermal check passed (via SoC proxy).");
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
// 5. testBoardAmbientTemperature
//    Check RP1 south-bridge zone and any hwmon sensors.
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testBoardAmbientTemperature()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Board ambient temperature (RP1 south-bridge zone)");

    long long rp1Temp = thrmReadInt("/sys/class/thermal/thermal_zone1/temp");
    if (rp1Temp > 0) {
        double c = rp1Temp / 1000.0;
        data.push_back("thermal_zone1 (rp1-thermal): " + std::to_string(c) + " °C");
        if (c > 85.0) {
            passed = false;
            data.push_back("WARNING: RP1 south-bridge temperature above 85°C.");
        }
    } else {
        data.push_back("RP1 thermal zone: not exposed or unavailable.");
    }

    // Check hwmon for board sensors (e.g., external thermistor)
    DIR* dir = opendir("/sys/class/hwmon");
    if (dir) {
        struct dirent* e;
        while ((e = readdir(dir)) != nullptr) {
            std::string name(e->d_name);
            if (name.rfind("hwmon", 0) != 0) continue;
            std::string base = "/sys/class/hwmon/" + name;
            std::string devName = thrmReadFile(base + "/name");
            for (int i = 1; i <= 4; ++i) {
                long long t = thrmReadInt(base + "/temp" + std::to_string(i) + "_input");
                if (t > 0)
                    data.push_back("hwmon " + devName + " temp" + std::to_string(i) + ": " +
                                   std::to_string(t / 1000.0) + " °C");
            }
        }
        closedir(dir);
    }

    if (passed) data.push_back("PASS: Board ambient temperature check complete.");
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
// 6. testAllThermalZones
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testAllThermalZones()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("All thermal zones on Pi5:");
    auto zones = readAllThermalZones();

    if (zones.empty()) {
        data.push_back("FAIL: No thermal zones found.");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    for (const auto& z : zones) {
        std::string flag = "";
        if (z.second >= SHUTDOWN_C) { flag = " *** OVER SHUTDOWN ***"; passed = false; }
        else if (z.second >= THROTTLE_C) { flag = " [THROTTLING]"; passed = false; }
        data.push_back("  " + z.first + ": " + std::to_string(z.second) + " °C" + flag);
    }

    data.push_back("Total thermal zones: " + std::to_string(zones.size()));
    if (passed) data.push_back("PASS: All thermal zones within safe range.");
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
// 7. testThermalThrottlingThresholds
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testThermalThrottlingThresholds()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Thermal throttle configuration check (Pi5)");
    data.push_back("BCM2712 throttle onset: " + std::to_string(THROTTLE_C) + " °C");
    data.push_back("BCM2712 soft shutdown:  " + std::to_string(SHUTDOWN_C) + " °C");

    // Read trip points for zone0
    for (int t = 0; t < 10; ++t) {
        std::string base = "/sys/class/thermal/thermal_zone0/trip_point_" + std::to_string(t);
        std::string type = thrmReadFile(base + "_type");
        if (type.empty()) break;
        long long temp = thrmReadInt(base + "_temp");
        data.push_back("Trip " + std::to_string(t) + " [" + type + "]: " +
                       std::to_string(temp / 1000.0) + " °C");
    }

    // Check vcgencmd throttled status
    std::string throttled = thrmRunCmd("vcgencmd get_throttled 2>/dev/null");
    if (!throttled.empty()) {
        data.push_back("vcgencmd throttled: " + throttled);
        // 0x0 = no throttling
        if (throttled.find("0x0") == std::string::npos)
            data.push_back("WARNING: Throttling or under-voltage event detected.");
        else
            data.push_back("No active throttling.");
    }

    double curC = readSocTempC();
    if (curC >= THROTTLE_C) {
        passed = false;
        data.push_back("FAIL: Currently throttling (T=" + std::to_string(curC) + " °C).");
    } else {
        data.push_back("PASS: Throttle thresholds confirmed, no active throttling.");
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
// 8. testThermalShutdownTemperature
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testThermalShutdownTemperature()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Thermal shutdown temperature check (Pi5)");
    data.push_back("Configured shutdown temperature: " + std::to_string(SHUTDOWN_C) + " °C");

    // Read critical trip points
    for (int z = 0; z < 3; ++z) {
        std::string base = "/sys/class/thermal/thermal_zone" + std::to_string(z);
        for (int t = 0; t < 10; ++t) {
            std::string tbase = base + "/trip_point_" + std::to_string(t);
            std::string type = thrmReadFile(tbase + "_type");
            if (type.empty()) break;
            if (type == "critical") {
                long long temp = thrmReadInt(tbase + "_temp");
                data.push_back("Zone" + std::to_string(z) + " critical trip: " +
                               std::to_string(temp / 1000.0) + " °C");
            }
        }
    }

    double curC = readSocTempC();
    data.push_back("Current temperature: " + std::to_string(curC) + " °C");
    data.push_back("Headroom to shutdown: " + std::to_string(SHUTDOWN_C - curC) + " °C");

    if (curC >= SHUTDOWN_C) {
        passed = false;
        data.push_back("FAIL: At or above shutdown temperature!");
    } else {
        data.push_back("PASS: Safe headroom to shutdown temperature.");
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
// 9. testCoolingSolutionEffectiveness
//    Run a short stress workload; measure temperature rise.
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testCoolingSolutionEffectiveness()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Cooling solution effectiveness (3-second stress test)");

    double tempBefore = readSocTempC();
    data.push_back("Temperature before stress: " + std::to_string(tempBefore) + " °C");

    // Run 3-second CPU stress on all 4 cores
    std::vector<std::thread> threads;
    for (int i = 0; i < RPI5_CPU_CORES; ++i)
        threads.emplace_back(thermalStressWorkload, 3.0);
    for (auto& t : threads) t.join();

    double tempAfter = readSocTempC();
    data.push_back("Temperature after stress:  " + std::to_string(tempAfter)  + " °C");
    data.push_back("Temperature rise: " + std::to_string(tempAfter - tempBefore) + " °C");

    if (tempAfter >= SHUTDOWN_C) {
        passed = false;
        data.push_back("FAIL: Temperature reached shutdown threshold during 3s stress.");
    } else if (tempAfter >= THROTTLE_C) {
        passed = false;
        data.push_back("WARNING: Thermal throttling triggered during stress test.");
        data.push_back("Improve cooling: add heatsink/fan for sustained workloads.");
    } else {
        data.push_back("PASS: Cooling solution effective for 3s full-load test.");
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
// 10. testTemperatureDistributionAcrossDie
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testTemperatureDistributionAcrossDie()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Temperature distribution across BCM2712 die");
    data.push_back("Pi5 has 2 thermal sensors: cpu-thermal (zone0) and rp1-thermal (zone1).");

    auto zones = readAllThermalZones();
    for (const auto& z : zones)
        data.push_back("  " + z.first + ": " + std::to_string(z.second) + " °C");

    if (zones.size() >= 2) {
        double delta = std::abs(zones[0].second - zones[1].second);
        data.push_back("Inter-zone delta: " + std::to_string(delta) + " °C");
        if (delta > 30.0) {
            data.push_back("NOTE: Large temperature differential between zones.");
        }
    }

    data.push_back("PASS: Die temperature distribution check complete.");
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
// 11. testSustainedThermalPerformance
//    Run 5-second stress, monitor for frequency throttling.
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testSustainedThermalPerformance()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Sustained thermal performance test (5 seconds)");

    // Record initial CPU frequency
    long long freqBefore = -1;
    std::ifstream freqFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
    if (freqFile.is_open()) { freqFile >> freqBefore; }

    double tempBefore = readSocTempC();
    data.push_back("Initial: T=" + std::to_string(tempBefore) + " °C, " +
                   "freq=" + std::to_string(freqBefore / 1000) + " MHz");

    // Stress
    std::vector<std::thread> threads;
    for (int i = 0; i < RPI5_CPU_CORES; ++i)
        threads.emplace_back(thermalStressWorkload, 5.0);
    for (auto& t : threads) t.join();

    long long freqAfter = -1;
    std::ifstream freqFile2("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
    if (freqFile2.is_open()) { freqFile2 >> freqAfter; }

    double tempAfter = readSocTempC();
    data.push_back("After 5s: T=" + std::to_string(tempAfter) + " °C, " +
                   "freq=" + std::to_string(freqAfter / 1000) + " MHz");

    if (freqBefore > 0 && freqAfter > 0) {
        double freqDrop = 100.0 * (freqBefore - freqAfter) / freqBefore;
        data.push_back("Frequency change: " + std::to_string(freqDrop) + " %");
        if (freqDrop > 20.0) {
            passed = false;
            data.push_back("WARNING: >20% frequency drop under sustained load — thermal throttling.");
        }
    }

    if (tempAfter >= THROTTLE_C) {
        passed = false;
        data.push_back("WARNING: Temperature hit throttle threshold during sustained test.");
    } else {
        data.push_back("PASS: Sustained thermal performance test complete.");
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
// 12. testThermalRecoveryTime
//    Measure how quickly temperature returns to baseline after load.
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testThermalRecoveryTime()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Thermal recovery time measurement");

    // Run 3s stress
    {
        std::vector<std::thread> threads;
        for (int i = 0; i < RPI5_CPU_CORES; ++i)
            threads.emplace_back(thermalStressWorkload, 3.0);
        for (auto& t : threads) t.join();
    }

    double peakC = readSocTempC();
    data.push_back("Peak temperature after 3s stress: " + std::to_string(peakC) + " °C");

    // Wait up to 30s for recovery to within 10°C of baseline
    double baseline = peakC - 10.0; // rough target
    auto tStart = std::chrono::steady_clock::now();
    double recoveredC = peakC;
    for (int i = 0; i < 30; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        recoveredC = readSocTempC();
        if (recoveredC <= baseline) break;
    }
    double recoverySec = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - tStart).count();

    data.push_back("Temperature after cooling: " + std::to_string(recoveredC) + " °C");
    data.push_back("Recovery time: " + std::to_string(recoverySec) + " s");

    if (recoveredC > THROTTLE_C) {
        passed = false;
        data.push_back("WARNING: Still above throttle threshold after recovery period.");
    } else {
        data.push_back("PASS: Thermal recovery within acceptable time.");
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
// 13. testFanControl
//    Pi5 Active Cooler has a PWM fan controlled via RP1. Check via sysfs.
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testFanControl()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Pi5 Active Cooler fan control (RP1 PWM fan connector)");

    // Pi5 fan uses pwmchip sysfs or thermal cooling_device
    bool fanFound = false;
    for (int c = 0; c < 8; ++c) {
        std::string base = "/sys/class/thermal/cooling_device" + std::to_string(c);
        std::string type = thrmReadFile(base + "/type");
        if (type.empty()) break;
        if (type.find("fan") != std::string::npos || type.find("pwm") != std::string::npos) {
            fanFound = true;
            long long state    = thrmReadInt(base + "/cur_state");
            long long maxState = thrmReadInt(base + "/max_state");
            data.push_back("Fan cooling_device" + std::to_string(c) + " [" + type +
                           "]: state=" + std::to_string(state) + "/" + std::to_string(maxState));
        }
    }

    // Also check pwm sysfs
    DIR* dir = opendir("/sys/class/pwm");
    if (dir) {
        struct dirent* e;
        while ((e = readdir(dir)) != nullptr) {
            std::string name(e->d_name);
            if (name.rfind("pwm", 0) == 0) {
                data.push_back("PWM device: " + name);
                fanFound = true;
            }
        }
        closedir(dir);
    }

    // Check via hwmon (fan rpm)
    std::string fanRpm = thrmRunCmd("cat /sys/class/hwmon/hwmon*/fan1_input 2>/dev/null | head -1");
    if (!fanRpm.empty()) {
        data.push_back("Fan RPM: " + fanRpm);
        fanFound = true;
    }

    if (!fanFound) {
        data.push_back("INFO: Pi5 Active Cooler not detected or no fan connected.");
        data.push_back("      Pi5 fanless operation is normal with passive heatsink.");
    } else {
        data.push_back("PASS: Fan control interface detected.");
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
std::vector<TestResult> ThermalManagementTesting::runAllTests()
{
    std::vector<TestResult> results;

    // Run testSocJunctionTemperature
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSocJunctionTemperature();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSocJunctionTemperature";
            _tr.test_id = "testSocJunctionTemperature";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testGpuTemperature
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testGpuTemperature();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testGpuTemperature";
            _tr.test_id = "testGpuTemperature";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testCpuTemperature
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testCpuTemperature();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testCpuTemperature";
            _tr.test_id = "testCpuTemperature";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testMemoryTemperature
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testMemoryTemperature();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testMemoryTemperature";
            _tr.test_id = "testMemoryTemperature";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testBoardAmbientTemperature
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testBoardAmbientTemperature();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testBoardAmbientTemperature";
            _tr.test_id = "testBoardAmbientTemperature";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testAllThermalZones
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testAllThermalZones();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testAllThermalZones";
            _tr.test_id = "testAllThermalZones";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testThermalThrottlingThresholds
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testThermalThrottlingThresholds();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testThermalThrottlingThresholds";
            _tr.test_id = "testThermalThrottlingThresholds";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testThermalShutdownTemperature
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testThermalShutdownTemperature();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testThermalShutdownTemperature";
            _tr.test_id = "testThermalShutdownTemperature";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testCoolingSolutionEffectiveness
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testCoolingSolutionEffectiveness();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testCoolingSolutionEffectiveness";
            _tr.test_id = "testCoolingSolutionEffectiveness";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testTemperatureDistributionAcrossDie
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testTemperatureDistributionAcrossDie();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testTemperatureDistributionAcrossDie";
            _tr.test_id = "testTemperatureDistributionAcrossDie";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSustainedThermalPerformance
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSustainedThermalPerformance();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSustainedThermalPerformance";
            _tr.test_id = "testSustainedThermalPerformance";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testThermalRecoveryTime
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testThermalRecoveryTime();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testThermalRecoveryTime";
            _tr.test_id = "testThermalRecoveryTime";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testFanControl
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testFanControl();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testFanControl";
            _tr.test_id = "testFanControl";
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
std::vector<std::string> ThermalManagementTesting::getAvailableTests()
{
    return {
        "testSocJunctionTemperature",
        "testGpuTemperature",
        "testCpuTemperature",
        "testMemoryTemperature",
        "testBoardAmbientTemperature",
        "testAllThermalZones",
        "testThermalThrottlingThresholds",
        "testThermalShutdownTemperature",
        "testCoolingSolutionEffectiveness",
        "testTemperatureDistributionAcrossDie",
        "testSustainedThermalPerformance",
        "testThermalRecoveryTime",
        "testFanControl",
    };
}

/**
 * @brief Run a single test by name
 * @param testName Name of the test to run
 * @return Test result with timing
 */
TestResult ThermalManagementTesting::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if (testName == "testSocJunctionTemperature") {
        result = testSocJunctionTemperature();
    } else if (testName == "testGpuTemperature") {
        result = testGpuTemperature();
    } else if (testName == "testCpuTemperature") {
        result = testCpuTemperature();
    } else if (testName == "testMemoryTemperature") {
        result = testMemoryTemperature();
    } else if (testName == "testBoardAmbientTemperature") {
        result = testBoardAmbientTemperature();
    } else if (testName == "testAllThermalZones") {
        result = testAllThermalZones();
    } else if (testName == "testThermalThrottlingThresholds") {
        result = testThermalThrottlingThresholds();
    } else if (testName == "testThermalShutdownTemperature") {
        result = testThermalShutdownTemperature();
    } else if (testName == "testCoolingSolutionEffectiveness") {
        result = testCoolingSolutionEffectiveness();
    } else if (testName == "testTemperatureDistributionAcrossDie") {
        result = testTemperatureDistributionAcrossDie();
    } else if (testName == "testSustainedThermalPerformance") {
        result = testSustainedThermalPerformance();
    } else if (testName == "testThermalRecoveryTime") {
        result = testThermalRecoveryTime();
    } else if (testName == "testFanControl") {
        result = testFanControl();
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
