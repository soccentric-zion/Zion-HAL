/**
 * @file reliability_lib.cpp
 * @brief Reliability tests for Raspberry Pi 5 (BCM2712).
 *        Tests characterise the long-term health of the board by examining
 *        system uptime, thermal headroom, power-supply stability indicators,
 *        and storage health metrics accessible via kernel sysfs/procfs.
 *        Physical burn-in, solder-joint, and thermal-cycling procedures cannot
 *        be performed programmatically; those tests report observable proxies.
 * @author sandesh ghimire
 * @copyright (C) Lucid Motors
 */

#include "reliability_lib.h"
#include "../platform_rpi5.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string relReadFile(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string s = ss.str();
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' '))
        s.pop_back();
    return s;
}

static std::string relRunCmd(const std::string& cmd)
{
    std::array<char, 512> buf{};
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    while (fgets(buf.data(), buf.size(), pipe))
        result += buf.data();
    pclose(pipe);
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
        result.pop_back();
    return result;
}

// ---------------------------------------------------------------------------

/**
 * @brief Constructor for ReliabilityTesting
 */
ReliabilityTesting::ReliabilityTesting()
{
}

/**
 * @brief Destructor for ReliabilityTesting
 */
ReliabilityTesting::~ReliabilityTesting()
{
}

/**
 * @brief Burn-in proxy: brief CPU+memory stress and throttle check.
 */
TestOutput ReliabilityTesting::testBurninTesting()
{
    std::vector<std::string> data;
    data.push_back("=== Burn-In Proxy (5-second stress, Pi5) ===");

    std::string throttlePre = relRunCmd("vcgencmd get_throttled 2>/dev/null");
    data.push_back("[INFO] Throttle before: " + (throttlePre.empty() ? "N/A" : throttlePre));

    // 5-second stress across 4 cores
    int rc = system("for i in 1 2 3 4; do (dd if=/dev/zero bs=1M count=512 2>/dev/null | md5sum &); done; sleep 5; kill $(jobs -p) 2>/dev/null; true");
    (void)rc;

    std::string throttlePost = relRunCmd("vcgencmd get_throttled 2>/dev/null");
    data.push_back("[INFO] Throttle after:  " + (throttlePost.empty() ? "N/A" : throttlePost));

    std::string tempStr = relReadFile("/sys/class/thermal/thermal_zone0/temp");
    if (!tempStr.empty())
    {
        double tempC = std::stod(tempStr) / 1000.0;
        data.push_back("[INFO] SoC temp after stress: " + std::to_string(tempC) + " °C");
        data.push_back(tempC < static_cast<double>(RPI5_THERMAL_THROTTLE_C)
                           ? "[PASS] Below throttle threshold"
                           : "[WARN] At/above throttle threshold");
    }

    bool uvDetected = !throttlePost.empty() && throttlePost != "throttled=0x0";
    data.push_back(uvDetected ? "[WARN] Throttle flag set — check power supply"
                              : "[PASS] No throttle flags after stress");
    data.push_back("[INFO] Full burn-in (hours) requires dedicated test fixture");
    {
        TestOutput _out;
        _out.status = !uvDetected ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = !uvDetected ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Power cycling proxy: uptime, boot count, and filesystem health.
 */
TestOutput ReliabilityTesting::testPowerCycling()
{
    std::vector<std::string> data;
    data.push_back("=== Power Cycling Resilience (Pi5) ===");

    std::string uptime = relReadFile("/proc/uptime");
    if (!uptime.empty())
    {
        double sec = std::stod(uptime);
        data.push_back("[PASS] Uptime: " + std::to_string(static_cast<int>(sec)) + "s");
    }

    std::string bootCount = relRunCmd("last reboot 2>/dev/null | grep -c '^reboot'");
    data.push_back("[INFO] Boot entries in wtmp: " + bootCount);

    std::string panics = relRunCmd("journalctl -k --no-pager 2>/dev/null | grep -c -i 'kernel panic\\|Oops'");
    data.push_back(panics == "0" ? "[PASS] No kernel panics in journal"
                                 : "[WARN] Kernel panic count: " + panics);

    std::string fsErr = relRunCmd("dmesg 2>/dev/null | grep -c -i 'EXT4-fs error\\|Buffer I/O error'");
    data.push_back(fsErr == "0" ? "[PASS] No FS errors in dmesg"
                                : "[WARN] FS error count: " + fsErr);

    data.push_back("[INFO] Physical power-cycle test requires external controller");
    {
        TestOutput _out;
        _out.status = hal::Status::PASS;
        _out.severity = hal::Severity::INFO;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Thermal cycling proxy: verify SoC temperature is within rated range.
 */
TestOutput ReliabilityTesting::testThermalCycling()
{
    std::vector<std::string> data;
    data.push_back("=== Thermal Cycling Proxy (Pi5) ===");
    data.push_back("[INFO] Pi5 BCM2712 rated: 0–85 °C junction");

    bool allOk = true;
    for (int zone = 0; zone <= 1; zone++)
    {
        std::string base    = "/sys/class/thermal/thermal_zone" + std::to_string(zone);
        std::string tempStr = relReadFile(base + "/temp");
        std::string type    = relReadFile(base + "/type");
        if (tempStr.empty()) continue;
        double tempC = std::stod(tempStr) / 1000.0;
        bool ok      = tempC >= 0.0 && tempC < static_cast<double>(RPI5_THERMAL_SHUTDOWN_C);
        data.push_back((ok ? "[PASS] " : "[WARN] ") + type + ": " +
                       std::to_string(tempC) + " °C " + (ok ? "(in spec)" : "(out of spec!)"));
        if (!ok) allOk = false;
    }

    data.push_back("[INFO] Active thermal cycling requires HALT/HASS chamber");
    {
        TestOutput _out;
        _out.status = allOk ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = allOk ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief MTBF proxy: uptime, pstore, and PCIe AER error counters.
 */
TestOutput ReliabilityTesting::testMtbf()
{
    std::vector<std::string> data;
    data.push_back("=== MTBF Proxy (Pi5) ===");

    std::string uptimeStr = relReadFile("/proc/uptime");
    if (!uptimeStr.empty())
        data.push_back("[INFO] Uptime: " + std::to_string(static_cast<long>(std::stod(uptimeStr))) + "s");

    data.push_back("[INFO] Pi5 has no on-die ECC DRAM; MTBF via hardware fault counters only");

    std::string pstoreCount = relRunCmd("ls /sys/fs/pstore/ 2>/dev/null | wc -l");
    bool pstoreOops = std::stoi(pstoreCount.empty() ? "0" : pstoreCount) > 0;
    data.push_back(pstoreOops ? "[WARN] pstore crash entries: " + pstoreCount
                              : "[PASS] pstore empty — no prior crash records");

    std::string aerErr = relRunCmd(
        "grep -r '' /sys/bus/pci/devices/*/aer_stats/ 2>/dev/null | grep -v ':0$' | head -5");
    data.push_back(aerErr.empty() ? "[PASS] No PCIe AER errors"
                                  : "[WARN] PCIe AER errors:\n" + aerErr);

    {
        TestOutput _out;
        _out.status = !pstoreOops ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = !pstoreOops ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Component aging proxy: storage endurance and CPU frequency headroom.
 */
TestOutput ReliabilityTesting::testComponentAging()
{
    std::vector<std::string> data;
    data.push_back("=== Component Aging Proxy (Pi5) ===");

    std::string mmcStat = relReadFile("/sys/block/mmcblk0/stat");
    if (!mmcStat.empty())
        data.push_back("[INFO] mmcblk0 stats: " + mmcStat);

    std::string nvmeSmart = relRunCmd(
        "nvme smart-log /dev/nvme0 2>/dev/null | grep -E 'percentage_used|power_on_hours'");
    data.push_back(nvmeSmart.empty() ? "[INFO] NVMe SMART unavailable"
                                     : "[INFO] NVMe SMART:\n" + nvmeSmart);

    std::string maxF = relReadFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");
    std::string curF = relReadFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
    if (!maxF.empty() && !curF.empty())
    {
        long mf  = std::stol(maxF);
        long cf  = std::stol(curF);
        double p = 100.0 * cf / mf;
        data.push_back("[INFO] CPU0 freq: " + std::to_string(cf / 1000) + " / " +
                       std::to_string(mf / 1000) + " MHz (" + std::to_string(static_cast<int>(p)) + "%)");
    }

    {
        TestOutput _out;
        _out.status = hal::Status::PASS;
        _out.severity = hal::Severity::INFO;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Solder joint integrity proxy: PCIe link state and USB/eMMC errors.
 */
TestOutput ReliabilityTesting::testSolderJointIntegrity()
{
    std::vector<std::string> data;
    data.push_back("=== Solder Joint Integrity Proxy (Pi5) ===");
    data.push_back("[INFO] Physical testing requires X-ray / thermal imaging");

    std::string pcieLinks = relRunCmd(
        "for d in /sys/bus/pci/devices/*; do s=$(cat $d/current_link_speed 2>/dev/null); "
        "[ -n \"$s\" ] && echo \"$(basename $d): $s\"; done 2>/dev/null");
    data.push_back(pcieLinks.empty() ? "[INFO] No PCIe devices"
                                     : "[PASS] PCIe links:\n" + pcieLinks);

    std::string usbResets = relRunCmd(
        "dmesg 2>/dev/null | grep -c -i 'USB disconnect\\|reset full-speed\\|xhci.*error'");
    data.push_back(usbResets == "0" ? "[PASS] No USB error events"
                                    : "[WARN] USB errors: " + usbResets);

    std::string mmcErr = relRunCmd("dmesg 2>/dev/null | grep -c -i 'mmc.*error\\|mmcblk.*error'");
    data.push_back(mmcErr == "0" ? "[PASS] No eMMC/SD errors in dmesg"
                                 : "[WARN] eMMC/SD errors: " + mmcErr);

    {
        TestOutput _out;
        _out.status = hal::Status::PASS;
        _out.severity = hal::Severity::INFO;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Run all tests for this module
 * @return Vector of test results
 */
std::vector<TestResult> ReliabilityTesting::runAllTests()
{
    std::vector<TestResult> results;

    // Run testBurninTesting
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testBurninTesting();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testBurninTesting";
            _tr.test_id = "testBurninTesting";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testPowerCycling
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPowerCycling();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPowerCycling";
            _tr.test_id = "testPowerCycling";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testThermalCycling
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testThermalCycling();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testThermalCycling";
            _tr.test_id = "testThermalCycling";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testMtbf
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testMtbf();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testMtbf";
            _tr.test_id = "testMtbf";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testComponentAging
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testComponentAging();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testComponentAging";
            _tr.test_id = "testComponentAging";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSolderJointIntegrity
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSolderJointIntegrity();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSolderJointIntegrity";
            _tr.test_id = "testSolderJointIntegrity";
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
std::vector<std::string> ReliabilityTesting::getAvailableTests()
{
    return {
        "testBurninTesting", "testPowerCycling", "testThermalCycling", "testMtbf", "testComponentAging", "testSolderJointIntegrity",
    };
}

/**
 * @brief Run a single test by name
 * @param testName Name of the test to run
 * @return Test result with timing
 */
TestResult ReliabilityTesting::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if (testName == "testBurninTesting") {
        result = testBurninTesting();
    } else if (testName == "testPowerCycling") {
        result = testPowerCycling();
    } else if (testName == "testThermalCycling") {
        result = testThermalCycling();
    } else if (testName == "testMtbf") {
        result = testMtbf();
    } else if (testName == "testComponentAging") {
        result = testComponentAging();
    } else if (testName == "testSolderJointIntegrity") {
        result = testSolderJointIntegrity();
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
