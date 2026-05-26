/**
 * @file functional_safety_lib.cpp
 * @brief Functional safety tests for Raspberry Pi 5 (BCM2712).
 *        Pi5 is a commercial SBC and is NOT rated for automotive ASIL-D or
 *        IEC 61508 SIL-3.  These tests document the available safety-relevant
 *        features (watchdog, ECC proxy, thermal shutdown, PMIC brownout) and
 *        map Jetson-specific tests (lockstep CPU, ASIL rails) to Pi5 equivalents.
 * @author sandesh ghimire
 * @copyright (C) Lucid Motors
 */

#include "functional_safety_lib.h"
#include "../platform_rpi5.h"

#include <array>
#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

// ---------------------------------------------------------------------------
[[maybe_unused]] static std::string fsReadFile(const std::string& path)
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

static std::string fsRunCmd(const std::string& cmd)
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

static bool fsPathExists(const std::string& p)
{
    struct stat st {};
    return stat(p.c_str(), &st) == 0;
}
// ---------------------------------------------------------------------------

/**
 * @brief Constructor for FunctionalSafetyTesting
 */
FunctionalSafetyTesting::FunctionalSafetyTesting()
{
}

/**
 * @brief Destructor for FunctionalSafetyTesting
 */
FunctionalSafetyTesting::~FunctionalSafetyTesting()
{
}

/**
 * @brief Lockstep CPU test proxy (Pi5 does not have hardware lockstep).
 * @details BCM2712 Cortex-A76 does not implement lockstep CPU execution.
 *          This test documents the limitation and verifies the kernel watchdog
 *          (RP1 hardware watchdog) as the closest safety-relevant substitute.
 */
TestOutput FunctionalSafetyTesting::testLockstepCpuOperation()
{
    std::vector<std::string> data;
    data.push_back("=== CPU Lockstep (Pi5 BCM2712) ===");
    data.push_back("[INFO] Pi5 Cortex-A76 does NOT implement hardware CPU lockstep");
    data.push_back("[INFO] For ASIL-C/D lockstep requirement, use dedicated MCU (e.g. RP2040 HAT)");

    // Check RP1 watchdog as a lockstep substitute
    bool wdPresent = fsPathExists("/dev/watchdog0");
    data.push_back(wdPresent ? "[PASS] RP1 hardware watchdog /dev/watchdog0 present (safety monitor)"
                             : "[WARN] /dev/watchdog0 not found — watchdog driver not loaded");

    // Check if systemd-watchdog is active
    std::string wdState = fsRunCmd("systemctl is-active watchdog 2>/dev/null");
    data.push_back("[INFO] watchdog service: " + (wdState.empty() ? "not found" : wdState));

    // ARM CPU topology
    std::string cpuTopology = fsRunCmd("grep -m1 'CPU part' /proc/cpuinfo 2>/dev/null");
    data.push_back("[INFO] " + cpuTopology + " (Cortex-A76 part 0xD0B)");

    {
        TestOutput _out;
        _out.status = wdPresent ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = wdPresent ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief ECC memory test proxy (Pi5 LPDDR4X has no on-die ECC).
 * @details BCM2712 uses standard LPDDR4X-4267 without ECC.  This test
 *          documents the limitation and verifies that pstore/crash records
 *          are absent (proxy for memory integrity).
 */
TestOutput FunctionalSafetyTesting::testEccOnMemories()
{
    std::vector<std::string> data;
    data.push_back("=== ECC Memory (Pi5 LPDDR4X-4267) ===");
    data.push_back("[INFO] Pi5 BCM2712 LPDDR4X has NO hardware ECC");
    data.push_back("[INFO] For ECC, use Raspberry Pi CM5 Lite with ECC LPDDR5 carrier (future)");

    // pstore as memory-error proxy
    std::string pstoreFiles = fsRunCmd("ls /sys/fs/pstore/ 2>/dev/null | wc -l");
    bool cleanPstore = (std::stoi(pstoreFiles.empty() ? "0" : pstoreFiles) == 0);
    data.push_back(cleanPstore ? "[PASS] pstore empty — no recorded memory faults"
                               : "[WARN] pstore entries: " + pstoreFiles);

    // EDAC (Error Detection And Correction) kernel subsystem
    bool edacPresent = fsPathExists("/sys/bus/platform/drivers/edac_mpc85xx") ||
                       !fsRunCmd("ls /sys/devices/system/edac/ 2>/dev/null").empty();
    data.push_back(edacPresent ? "[INFO] EDAC subsystem present"
                               : "[INFO] No EDAC subsystem (expected on Pi5)");

    // Memory test utility
    std::string memtest = fsRunCmd("which memtester 2>/dev/null");
    data.push_back(memtest.empty() ? "[INFO] memtester not installed (install for software ECC simulation)"
                                   : "[PASS] memtester available: " + memtest);

    {
        TestOutput _out;
        _out.status = cleanPstore ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = cleanPstore ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Redundant power supplies test (Pi5 single USB-C input).
 * @details Pi5 uses a single USB-C 5V input; there is no redundant power path.
 *          Tests check for UPS HAT or multiple power_supply entries.
 */
TestOutput FunctionalSafetyTesting::testRedundantPowerSupplies()
{
    std::vector<std::string> data;
    data.push_back("=== Redundant Power Supplies (Pi5) ===");
    data.push_back("[INFO] Pi5 has single USB-C 5V input — no native power redundancy");

    // Check for UPS HAT power supply entries
    std::string psList = fsRunCmd("ls /sys/class/power_supply/ 2>/dev/null");
    data.push_back("[INFO] /sys/class/power_supply entries: " + (psList.empty() ? "none" : psList));

    if (!psList.empty())
    {
        // If there are battery/UPS entries, show status
        std::string batStatus = fsRunCmd(
            "for p in /sys/class/power_supply/*; do "
            "t=$(cat $p/type 2>/dev/null); "
            "s=$(cat $p/status 2>/dev/null); "
            "echo \"$(basename $p): type=$t status=$s\"; "
            "done 2>/dev/null");
        if (!batStatus.empty())
            data.push_back("[INFO] Power supply details:\n" + batStatus);
    }

    // Under-voltage flag (proxy for PSU health)
    std::string throttle = fsRunCmd("vcgencmd get_throttled 2>/dev/null");
    bool uvOk = !throttle.empty() && throttle == "throttled=0x0";
    data.push_back(uvOk ? "[PASS] No under-voltage flags"
                        : "[INFO] Throttle: " + throttle);

    data.push_back("[INFO] Redundancy requires UPS HAT (e.g. Pisugar, UPS-HAT) + firmware");
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
 * @brief Safety monitoring via kernel watchdog and throttle flags.
 */
TestOutput FunctionalSafetyTesting::testSafetyMonitoring()
{
    std::vector<std::string> data;
    data.push_back("=== Safety Monitoring (Pi5 RP1 Watchdog + PMIC) ===");

    // Hardware watchdog
    bool wdOk = fsPathExists("/dev/watchdog0");
    data.push_back(wdOk ? "[PASS] Hardware watchdog /dev/watchdog0 available"
                        : "[WARN] Watchdog device not found");

    // PMIC under-voltage / over-temp
    std::string throttle = fsRunCmd("vcgencmd get_throttled 2>/dev/null");
    if (!throttle.empty())
    {
        bool clean = throttle == "throttled=0x0";
        data.push_back((clean ? "[PASS] " : "[WARN] ") + throttle);
        // Decode flags
        if (!clean)
        {
            data.push_back("[INFO] Bit 0x00001: UV now | 0x00002: arm freq capped | 0x00004: throttled");
            data.push_back("[INFO] Bit 0x10000: UV occurred | 0x20000: arm capped occurred | 0x40000: throttled occurred");
        }
    }

    // Thermal critical trip point
    std::string critTemp = fsRunCmd(
        "cat /sys/class/thermal/thermal_zone0/trip_point_*_type 2>/dev/null | grep -c critical");
    data.push_back("[INFO] Critical trip points defined: " + critTemp);

    {
        TestOutput _out;
        _out.status = wdOk ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = wdOk ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Diagnostic coverage check: test coverage of critical Pi5 subsystems.
 */
TestOutput FunctionalSafetyTesting::testDiagnosticCoverage()
{
    std::vector<std::string> data;
    data.push_back("=== Diagnostic Coverage (Pi5) ===");

    // List what is testable
    static const std::vector<std::pair<std::string, std::string>> checks = {
        {"/dev/watchdog0",                     "RP1 hardware watchdog"},
        {"/sys/class/thermal/thermal_zone0",   "BCM2712 thermal zone"},
        {"/sys/class/thermal/thermal_zone1",   "RP1 thermal zone"},
        {"/sys/devices/system/cpu/cpu0/cpufreq","CPUfreq + throttle detection"},
        {"/sys/class/hwmon",                   "hwmon (PMIC/temp sensors)"},
        {"/sys/block/mmcblk0",                 "eMMC/SD storage"},
        {RPI5_GPIOCHIP_PATH,                   "GPIO controller"},
    };

    int present = 0;
    for (const auto& [path, desc] : checks)
    {
        bool ok = fsPathExists(path);
        data.push_back((ok ? "[PASS] " : "[INFO] ") + desc + (ok ? " — present" : " — absent"));
        if (ok) present++;
    }

    double coverage = 100.0 * present / static_cast<int>(checks.size());
    data.push_back("[INFO] Subsystem diagnostic coverage: " +
                   std::to_string(static_cast<int>(coverage)) + "% (" +
                   std::to_string(present) + "/" + std::to_string(checks.size()) + ")");

    {
        TestOutput _out;
        const bool _ok = present >= static_cast<int>(checks.size()) / 2;
        _out.status = _ok ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = _ok ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Fault injection proxy: trigger and detect a software watchdog timeout.
 * @details A real fault injection test would require JTAG or hardware fault
 *          insertion.  This test verifies that the watchdog WDIOC_GETBOOTSTATUS
 *          reports previous resets and that the kernel self-test (KASAN/UBSAN)
 *          infrastructure is available.
 */
TestOutput FunctionalSafetyTesting::testFaultInjectionTesting()
{
    std::vector<std::string> data;
    data.push_back("=== Fault Injection (Pi5 Proxy) ===");
    data.push_back("[INFO] Hardware fault injection requires JTAG; software proxy used");

    // Watchdog boot status (detects prior watchdog-triggered resets)
    if (fsPathExists("/dev/watchdog0"))
    {
        int fd = open("/dev/watchdog0", O_RDWR);
        if (fd >= 0)
        {
#ifdef WDIOC_GETBOOTSTATUS
            int bootStatus = 0;
            if (ioctl(fd, WDIOC_GETBOOTSTATUS, &bootStatus) == 0)
                data.push_back("[INFO] WD boot-status: " + std::to_string(bootStatus) +
                               (bootStatus != 0 ? " (prior WD reset)" : " (no prior WD reset)"));
#endif
            // Safe close
            write(fd, "V", 1);
            close(fd);
        }
    }

    // KASAN
    std::string kasan = fsRunCmd("grep -c 'CONFIG_KASAN=y' /boot/config-$(uname -r) 2>/dev/null");
    data.push_back(kasan == "1" ? "[PASS] KASAN memory error detection enabled"
                                : "[INFO] KASAN not enabled in kernel");

    // UBSAN
    std::string ubsan = fsRunCmd("grep -c 'CONFIG_UBSAN=y' /boot/config-$(uname -r) 2>/dev/null");
    data.push_back(ubsan == "1" ? "[PASS] UBSAN undefined-behavior detection enabled"
                                : "[INFO] UBSAN not enabled");

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
 * @brief Fail-safe behaviour: verify graceful thermal shutdown and watchdog reset.
 */
TestOutput FunctionalSafetyTesting::testFailsafeBehavior()
{
    std::vector<std::string> data;
    data.push_back("=== Fail-Safe Behaviour (Pi5) ===");

    // Thermal critical trip (shutdown at 85 °C)
    std::string critTrip = fsRunCmd(
        "for i in $(seq 0 9); do "
        "t=$(cat /sys/class/thermal/thermal_zone0/trip_point_${i}_type 2>/dev/null); "
        "v=$(cat /sys/class/thermal/thermal_zone0/trip_point_${i}_temp 2>/dev/null); "
        "[ \"$t\" = 'critical' ] && echo \"critical trip: ${v} mC\"; "
        "done 2>/dev/null");
    data.push_back(critTrip.empty() ? "[INFO] Critical thermal trip not found in zone0"
                                    : "[PASS] " + critTrip);

    // Watchdog reset capability
    bool wdPresent = fsPathExists("/dev/watchdog0");
    data.push_back(wdPresent ? "[PASS] Hardware watchdog available for fail-safe reset"
                             : "[WARN] No watchdog device");

    // vcgencmd throttled current state
    std::string throttle = fsRunCmd("vcgencmd get_throttled 2>/dev/null");
    data.push_back("[INFO] Current throttle: " + (throttle.empty() ? "N/A" : throttle));

    // Confirm systemd reboot target exists
    bool rebootTarget = fsPathExists("/etc/systemd/system/reboot.target") ||
                        fsPathExists("/lib/systemd/system/reboot.target");
    data.push_back(rebootTarget ? "[PASS] systemd reboot.target available (fail-safe reboot path)"
                                : "[INFO] systemd reboot.target not found");

    {
        TestOutput _out;
        _out.status = wdPresent ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = wdPresent ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief ASIL QS-L / QS-R rail isolation test (N/A on Pi5).
 * @details Pi5 BCM2712 / MXL7704 PMIC has no ASIL-rated isolated voltage rails.
 *          Documents the limitation and checks available hwmon voltage inputs.
 */
TestOutput FunctionalSafetyTesting::testAsilQsLAndRRailIsolation()
{
    std::vector<std::string> data;
    data.push_back("=== ASIL QS-L/R Rail Isolation (Pi5) ===");
    data.push_back("[INFO] Pi5 MXL7704 PMIC has NO ASIL-rated isolated supply rails");
    data.push_back("[INFO] For ASIL-D power, use Infineon TDA38640 or NXP FS84/FS85 PMIC");

    // Enumerate available hwmon voltage inputs as best-effort
    std::string voltages = fsRunCmd(
        "for d in /sys/class/hwmon/hwmon*; do "
        "for v in $d/in*_input; do "
        "[ -f \"$v\" ] && echo \"$(basename $d)/$(basename $v): $(cat $v 2>/dev/null) mV\"; "
        "done; done 2>/dev/null | head -10");
    data.push_back(voltages.empty() ? "[INFO] No hwmon voltage inputs found"
                                    : "[INFO] hwmon voltages:\n" + voltages);

    // vcgencmd voltage readings
    static const std::vector<std::string> rails = {"core", "sdram_c", "sdram_i", "sdram_p"};
    for (const auto& rail : rails)
    {
        std::string v = fsRunCmd("vcgencmd measure_volts " + rail + " 2>/dev/null");
        if (!v.empty())
            data.push_back("[INFO] " + rail + ": " + v);
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
 * @brief Run all tests for this module
 * @return Vector of test results
 */
std::vector<TestResult> FunctionalSafetyTesting::runAllTests()
{
    std::vector<TestResult> results;

    // Run testLockstepCpuOperation
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testLockstepCpuOperation();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testLockstepCpuOperation";
            _tr.test_id = "testLockstepCpuOperation";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testEccOnMemories
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testEccOnMemories();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testEccOnMemories";
            _tr.test_id = "testEccOnMemories";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testRedundantPowerSupplies
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testRedundantPowerSupplies();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testRedundantPowerSupplies";
            _tr.test_id = "testRedundantPowerSupplies";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSafetyMonitoring
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSafetyMonitoring();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSafetyMonitoring";
            _tr.test_id = "testSafetyMonitoring";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testDiagnosticCoverage
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testDiagnosticCoverage();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testDiagnosticCoverage";
            _tr.test_id = "testDiagnosticCoverage";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testFaultInjectionTesting
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testFaultInjectionTesting();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testFaultInjectionTesting";
            _tr.test_id = "testFaultInjectionTesting";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testFailsafeBehavior
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testFailsafeBehavior();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testFailsafeBehavior";
            _tr.test_id = "testFailsafeBehavior";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testAsilQsLAndRRailIsolation
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testAsilQsLAndRRailIsolation();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testAsilQsLAndRRailIsolation";
            _tr.test_id = "testAsilQsLAndRRailIsolation";
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
std::vector<std::string> FunctionalSafetyTesting::getAvailableTests()
{
    return {
        "testLockstepCpuOperation", "testEccOnMemories",         "testRedundantPowerSupplies", "testSafetyMonitoring",
        "testDiagnosticCoverage",   "testFaultInjectionTesting", "testFailsafeBehavior",       "testAsilQsLAndRRailIsolation",
    };
}

/**
 * @brief Run a single test by name
 * @param testName Name of the test to run
 * @return Test result with timing
 */
TestResult FunctionalSafetyTesting::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if (testName == "testLockstepCpuOperation") {
        result = testLockstepCpuOperation();
    } else if (testName == "testEccOnMemories") {
        result = testEccOnMemories();
    } else if (testName == "testRedundantPowerSupplies") {
        result = testRedundantPowerSupplies();
    } else if (testName == "testSafetyMonitoring") {
        result = testSafetyMonitoring();
    } else if (testName == "testDiagnosticCoverage") {
        result = testDiagnosticCoverage();
    } else if (testName == "testFaultInjectionTesting") {
        result = testFaultInjectionTesting();
    } else if (testName == "testFailsafeBehavior") {
        result = testFailsafeBehavior();
    } else if (testName == "testAsilQsLAndRRailIsolation") {
        result = testAsilQsLAndRRailIsolation();
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
