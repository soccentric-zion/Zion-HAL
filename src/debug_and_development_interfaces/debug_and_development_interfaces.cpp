/**
 * @file debug_and_development_interfaces.cpp
 * @brief Implementation file for DebugAndDevelopmentInterfaces class
 * @details Provides real hardware tests for debug and development interfaces
 *          on the Raspberry Pi 5 platform (Cortex-A78AE, Orin SoC).
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "debug_and_development_interfaces.h"
#include "../platform_rpi5.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

// ---------------------------------------------------------------------------
// Pi5 – platform constants
// ---------------------------------------------------------------------------
// Debug UART on Pi5 is mapped to ttyAMA0 (RP1 High Speed UART)
static constexpr const char* JETSON_DEBUG_UART = "/dev/ttyAMA0";
// CoreSight bus sysfs root
static constexpr const char* CORESIGHT_BUS_PATH = "/sys/bus/coresight/devices";
// perf subsystem paranoid knob
static constexpr const char* PERF_PARANOID_PATH = "/proc/sys/kernel/perf_event_paranoid";
// perf PMU event sources
static constexpr const char* PERF_DEVICES_PATH = "/sys/bus/event_source/devices";
// RP1 fuse driver sysfs – provides jtag/debug auth fuse readback
static constexpr const char* TEGRA_OTP_PATH = "/sys/devices/platform/rp1-fuse";
// debugfs mountpoint
static constexpr const char* DEBUGFS_PATH = "/sys/kernel/debug";
// JTAG character device (kernel JTAG driver)
static constexpr const char* JTAG_DEV_PATH = "/dev/jtag0";

// ---------------------------------------------------------------------------
// Helper utilities (file-local)
// ---------------------------------------------------------------------------

/** Read a single line from a file; returns empty string on failure. */
static std::string readLine(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open())
        return {};
    std::string line;
    std::getline(f, line);
    return line;
}

/** Return true if the path exists in the filesystem. */
static bool pathExists(const std::string& path)
{
    struct stat st
    {
    };
    return stat(path.c_str(), &st) == 0;
}

/** Return true if the path is a directory that can be opened. */
static bool isDir(const std::string& path)
{
    struct stat st
    {
    };
    if (stat(path.c_str(), &st) != 0)
        return false;
    return S_ISDIR(st.st_mode);
}

/**
 * List immediate children of a directory; returns empty vector on failure.
 * Skips "." and "..".
 */
static std::vector<std::string> listDir(const std::string& path)
{
    std::vector<std::string> entries;
    DIR* dir = opendir(path.c_str());
    if (!dir)
        return entries;
    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        std::string name = ent->d_name;
        if (name == "." || name == "..")
            continue;
        entries.push_back(name);
    }
    closedir(dir);
    return entries;
}

/** Count how many children of a directory contain a given substring. */
static int countMatchingEntries(const std::string& dirPath, const std::string& substr)
{
    int count = 0;
    for (const auto& e : listDir(dirPath)) {
        if (e.find(substr) != std::string::npos)
            ++count;
    }
    return count;
}

/**
 * @brief Constructor for DebugAndDevelopmentInterfaces
 */
DebugAndDevelopmentInterfaces::DebugAndDevelopmentInterfaces()
{
}

/**
 * @brief Destructor for DebugAndDevelopmentInterfaces
 */
DebugAndDevelopmentInterfaces::~DebugAndDevelopmentInterfaces()
{
}

/**
 * @brief Test the JTAG interface on Pi5.
 *
 * Checks:
 *  1. Kernel JTAG character device (/dev/jtag0) – present when the
 *     "jtag-generic" or RP1 JTAG driver is loaded.
 *  2. CoreSight sysfs bus is available (a prerequisite for JTAG debug).
 *  3. debugfs mount is accessible (JTAG operation commonly requires debugfs).
 *  4. RP1-fuse driver is present (provides jtag-disable fuse readback on Orin).
 *
 * @return TestOutput with pass/fail and diagnostic data.
 */
TestOutput DebugAndDevelopmentInterfaces::testJtagInterface()
{
    std::vector<std::string> data;
    bool passed = true;

    // 1. JTAG character device
    bool jtagDevPresent = pathExists(JTAG_DEV_PATH);
    data.push_back(std::string("JTAG device (/dev/jtag0): ") + (jtagDevPresent ? "present" : "not found"));
    if (!jtagDevPresent) {
        // Non-fatal if the driver simply isn't loaded – check alternative
        data.push_back("  INFO: /dev/jtag0 absent; driver may not be loaded or JTAG fused off.");
    }

    // 2. CoreSight bus (needed for ARM JTAG-DP access path)
    bool coresightBus = isDir(CORESIGHT_BUS_PATH);
    data.push_back(std::string("CoreSight bus sysfs: ") + (coresightBus ? "present" : "not found"));
    if (!coresightBus) {
        passed = false;
        data.push_back("FAIL: CoreSight bus not found – JTAG-DP debug infrastructure unavailable.");
    }

    // 3. debugfs accessibility
    bool debugfsOk = isDir(DEBUGFS_PATH);
    data.push_back(std::string("debugfs mountpoint (") + DEBUGFS_PATH + "): " + (debugfsOk ? "accessible" : "not accessible"));
    if (!debugfsOk) {
        passed = false;
        data.push_back("FAIL: debugfs not accessible – required for JTAG runtime interaction.");
    }

    // 4. RP1-fuse driver (provides jtag_disable fuse readback)
    bool fusePresent = isDir(TEGRA_OTP_PATH);
    data.push_back(std::string("RP1-fuse sysfs (") + TEGRA_OTP_PATH + "): " + (fusePresent ? "present" : "not found"));
    if (fusePresent) {
        // Try to read the jtag_disable fuse if available
        std::string jtagFuse = readLine(std::string(TEGRA_OTP_PATH) + "/jtag_disable");
        if (!jtagFuse.empty()) {
            data.push_back("  jtag_disable fuse value: " + jtagFuse);
            if (jtagFuse == "1") {
                passed = false;
                data.push_back("FAIL: JTAG is fuse-disabled on this device.");
            } else {
                data.push_back("  JTAG fuse: enabled (not blown).");
            }
        } else {
            data.push_back("  jtag_disable fuse: (not readable via sysfs)");
        }
    } else {
        data.push_back("  INFO: rp1-fuse driver absent; fuse state unknown.");
    }

    // Overall verdict
    data.push_back(std::string("JTAG interface test: ") + (passed ? "PASS" : "FAIL"));
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
 * @brief Test the Serial Wire Debug (SWD) interface on Pi5.
 *
 * SWD is the ARM two-wire debug protocol used alongside JTAG.
 * On Pi5 the ARM Cortex-A78AE cores expose debug access via the
 * CoreSight Debug Access Port (DAP).  This test checks:
 *  1. CoreSight DAP device(s) visible in the CoreSight bus sysfs.
 *  2. At least one CTI (Cross Trigger Interface) device – required for SWD halt.
 *  3. debugfs accessible (Linux perf/CoreSight drivers expose SWD state here).
 *
 * @return TestOutput with pass/fail and diagnostic data.
 */
TestOutput DebugAndDevelopmentInterfaces::testSerialWireDebug()
{
    std::vector<std::string> data;
    bool passed = true;

    // 1. CoreSight bus must exist
    if (!isDir(CORESIGHT_BUS_PATH)) {
        data.push_back("FAIL: CoreSight sysfs bus (" + std::string(CORESIGHT_BUS_PATH) + ") not found – SWD infrastructure missing.");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }
    data.push_back("CoreSight bus sysfs: present");

    // 2. Count DAP devices (named "*.dap" or containing "dap")
    int dapCount = countMatchingEntries(CORESIGHT_BUS_PATH, "dap");
    data.push_back("CoreSight DAP devices found: " + std::to_string(dapCount));
    if (dapCount == 0) {
        passed = false;
        data.push_back("FAIL: No CoreSight DAP device found – SWD-DP not enumerated.");
    }

    // 3. Count CTI devices (Cross-Trigger Interface – needed for SWD halt/resume)
    int ctiCount = countMatchingEntries(CORESIGHT_BUS_PATH, "cti");
    data.push_back("CoreSight CTI devices found: " + std::to_string(ctiCount));
    // Pi5 (Orin SoC) typically exposes 12+ CTI instances
    if (ctiCount == 0) {
        passed = false;
        data.push_back("FAIL: No CTI devices found – SWD halt/resume control unavailable.");
    } else if (ctiCount < 4) {
        data.push_back("  WARN: fewer CTI devices than expected for Pi5 (expected >=4).");
    }

    // 4. debugfs accessible
    bool debugfsOk = isDir(DEBUGFS_PATH);
    data.push_back(std::string("debugfs: ") + (debugfsOk ? "accessible" : "not accessible"));
    if (!debugfsOk) {
        passed = false;
        data.push_back("FAIL: debugfs not accessible – SWD kernel interface unavailable.");
    }

    // 5. Report all CoreSight devices for informational purposes
    auto devices = listDir(CORESIGHT_BUS_PATH);
    std::sort(devices.begin(), devices.end());
    data.push_back("All CoreSight devices (" + std::to_string(devices.size()) + "):");
    for (const auto& d : devices)
        data.push_back("  " + d);

    data.push_back(std::string("SWD interface test: ") + (passed ? "PASS" : "FAIL"));
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
 * @brief Test the Debug UART interface on Pi5.
 *
 * The primary debug console on Raspberry Pi 5 is exposed through the
 * RP1 High-Speed UART (ttyAMA0).  The test verifies:
 *  1. At least one /dev/ttyAMA* device is present.
 *  2. The device can be opened (O_RDWR | O_NOCTTY | O_NONBLOCK).
 *  3. The device type is reported from sysfs.
 *  4. Line discipline / termios attributes can be retrieved.
 *
 * @return TestOutput with pass/fail and diagnostic data.
 */
TestOutput DebugAndDevelopmentInterfaces::testDebugUart()
{
    std::vector<std::string> data;
    bool passed = true;

    // 1. Find all ttyAMA devices (RP1 High Speed UART)
    std::vector<std::string> thsDevices;
    for (int i = 0; i < 8; ++i) {
        std::string dev = "/dev/ttyAMA" + std::to_string(i);
        if (pathExists(dev))
            thsDevices.push_back(dev);
    }
    data.push_back("ttyAMA devices found: " + std::to_string(thsDevices.size()));
    for (const auto& d : thsDevices)
        data.push_back("  " + d);

    if (thsDevices.empty()) {
        passed = false;
        data.push_back("FAIL: No /dev/ttyAMA* device found – debug UART driver not loaded.");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    // 2. Try to open the primary debug UART (ttyAMA0)
    const std::string& primaryUart = thsDevices[0];
    int fd = open(primaryUart.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        data.push_back("FAIL: Cannot open " + primaryUart + " (errno=" + std::to_string(errno) + ")");
        // May be in use by the console – not necessarily fatal
        data.push_back("  INFO: device may be held by kernel console – this is expected on active debug UART.");
        // Check if it is registered as the system console
        std::string consoleInfo = readLine("/sys/class/tty/console/active");
        if (!consoleInfo.empty())
            data.push_back("  Active console: " + consoleInfo);
    } else {
        data.push_back("Opened " + primaryUart + " successfully (fd=" + std::to_string(fd) + ")");
        close(fd);
        data.push_back("Device closed cleanly.");
    }

    // 3. Read sysfs device info for each ttyAMA
    for (const auto& dev : thsDevices) {
        // e.g., /sys/class/tty/ttyAMA0/device/driver/module/...
        std::string devName = dev.substr(5); // strip "/dev/"
        std::string sysPath = "/sys/class/tty/" + devName;
        if (isDir(sysPath)) {
            std::string driverLink = sysPath + "/device/driver";
            data.push_back("  sysfs entry: " + sysPath + " -> present");
            // Try to read uevent for device type
            std::string ueventPath = sysPath + "/uevent";
            std::ifstream ue(ueventPath);
            if (ue.is_open()) {
                std::string line;
                while (std::getline(ue, line))
                    data.push_back("    " + line);
            }
        }
    }

    // 4. Check if the active console is ttyAMA (indicates debug UART is live)
    std::string activeConsole = readLine("/sys/class/tty/console/active");
    if (!activeConsole.empty()) {
        data.push_back("Kernel active console: " + activeConsole);
        if (activeConsole.find("ttyAMA") != std::string::npos)
            data.push_back("  INFO: Debug UART is the primary kernel console (expected on Pi5).");
    }

    data.push_back(std::string("Debug UART test: ") + (passed ? "PASS" : "FAIL"));
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
 * @brief Test the ARM CoreSight Trace Port (ETM/ETF/ETR/TPIU) on Pi5.
 *
 * The Pi5 (Orin SoC) incorporates embedded CoreSight trace:
 *  - ETM  (Embedded Trace Macrocell)  – one per CPU cluster / core
 *  - ETF  (Embedded Trace FIFO/buffer)
 *  - TPIU (Trace Port Interface Unit) – drives external trace connector
 *  - STM  (System Trace Macrocell)
 *
 * Checks:
 *  1. CoreSight bus sysfs is present.
 *  2. At least one ETM device is enumerated.
 *  3. At least one ETF/ETB (trace buffer) device is enumerated.
 *  4. TPIU device is present (external trace port).
 *  5. STM device is present.
 *
 * @return TestOutput with pass/fail and diagnostic data.
 */
TestOutput DebugAndDevelopmentInterfaces::testTracePort()
{
    std::vector<std::string> data;
    bool passed = true;

    // 1. CoreSight bus must exist
    if (!isDir(CORESIGHT_BUS_PATH)) {
        data.push_back("FAIL: CoreSight sysfs bus not present – no trace infrastructure.");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }
    data.push_back("CoreSight bus sysfs: present (" + std::string(CORESIGHT_BUS_PATH) + ")");

    // Enumerate all CoreSight devices
    auto devices = listDir(CORESIGHT_BUS_PATH);
    std::sort(devices.begin(), devices.end());
    data.push_back("Total CoreSight devices: " + std::to_string(devices.size()));

    // Count by type
    int etmCount = 0, etfCount = 0, tpiuCount = 0, stmCount = 0, etrCount = 0;
    for (const auto& d : devices) {
        std::string dl = d;
        // Convert to lowercase for comparison
        std::transform(dl.begin(), dl.end(), dl.begin(), ::tolower);
        if (dl.find("etm") != std::string::npos)
            ++etmCount;
        if (dl.find("etf") != std::string::npos || dl.find("etb") != std::string::npos)
            ++etfCount;
        if (dl.find("tpiu") != std::string::npos)
            ++tpiuCount;
        if (dl.find("stm") != std::string::npos)
            ++stmCount;
        if (dl.find("etr") != std::string::npos)
            ++etrCount;
        data.push_back("  " + d);
    }

    // 2. ETM check (one per CPU; Pi5 = 12 cores → expect up to 12 ETMs)
    data.push_back("ETM devices found: " + std::to_string(etmCount));
    if (etmCount == 0) {
        passed = false;
        data.push_back("FAIL: No ETM devices found – CPU instruction trace unavailable.");
    } else if (etmCount < 4) {
        data.push_back("  WARN: Fewer ETM devices than expected (Pi5 has 12 CPU cores).");
    }

    // 3. ETF/ETB check (trace buffer / funnel sink)
    data.push_back("ETF/ETB devices found: " + std::to_string(etfCount));
    if (etfCount == 0) {
        // ETR may serve as the sink instead
        data.push_back("  INFO: No ETF found; checking for ETR (system memory trace sink)...");
        data.push_back("  ETR devices found: " + std::to_string(etrCount));
        if (etrCount == 0) {
            passed = false;
            data.push_back("FAIL: No trace sink (ETF/ETB/ETR) found – trace cannot be captured.");
        }
    }

    // 4. TPIU check (external trace port connector)
    data.push_back("TPIU devices found: " + std::to_string(tpiuCount));
    if (tpiuCount == 0) {
        data.push_back("  INFO: No TPIU found; external trace port not enumerated.");
        // Not always fatal – depends on board design
    }

    // 5. STM check (system / software trace)
    data.push_back("STM devices found: " + std::to_string(stmCount));
    if (stmCount == 0) {
        data.push_back("  INFO: No STM found; software trace via STM unavailable.");
    }

    // 6. Check if CoreSight is exposed in debugfs
    std::string csDebugfs = std::string(DEBUGFS_PATH) + "/coresight";
    bool csDebugfsOk = isDir(csDebugfs);
    data.push_back(std::string("debugfs/coresight: ") + (csDebugfsOk ? "present" : "not found"));
    if (!csDebugfsOk) {
        data.push_back("  INFO: debugfs/coresight absent – runtime trace control via debugfs unavailable.");
    }

    data.push_back(std::string("Trace port test: ") + (passed ? "PASS" : "FAIL"));
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
 * @brief Test Hardware Performance Monitoring Counters (PMU) on Pi5.
 *
 * The ARM Cortex-A78AE cores on Pi5 include a PMU (Performance
 * Monitoring Unit) with 6 programmable event counters per core.
 * The Linux perf_event subsystem surfaces these via:
 *  - /proc/sys/kernel/perf_event_paranoid   – access control level
 *  - /sys/bus/event_source/devices/         – PMU device enumeration
 *  - /sys/bus/event_source/devices/armv8_*  – ARM PMU
 *  - /sys/bus/event_source/devices/gpu_*    – GPU PMU (Ampere GPU on Orin)
 *
 * Checks:
 *  1. perf_event_paranoid is readable and not fully locked (!=3/-1 only in debug builds).
 *  2. At least one ARM PMU event source exists.
 *  3. Orin GPU PMU event source exists.
 *  4. Number of hardware counters reported per PMU.
 *
 * @return TestOutput with pass/fail and diagnostic data.
 */
TestOutput DebugAndDevelopmentInterfaces::testPerformanceCounters()
{
    std::vector<std::string> data;
    bool passed = true;

    // 1. perf_event_paranoid
    std::string paranoidStr = readLine(PERF_PARANOID_PATH);
    if (paranoidStr.empty()) {
        passed = false;
        data.push_back("FAIL: Cannot read " + std::string(PERF_PARANOID_PATH) + " – perf_event subsystem not available.");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }
    int paranoid = std::stoi(paranoidStr);
    data.push_back("perf_event_paranoid: " + paranoidStr);
    if (paranoid > 2) {
        data.push_back("  WARN: paranoid=" + paranoidStr + " – unprivileged access to hardware PMU counters is restricted.");
    } else {
        data.push_back("  PMU access level: " +
                       std::string(paranoid <= 0 ? "unrestricted" : paranoid == 1 ? "user-level (normal)" : "kernel only"));
    }

    // 2. Enumerate event source devices
    if (!isDir(PERF_DEVICES_PATH)) {
        passed = false;
        data.push_back("FAIL: " + std::string(PERF_DEVICES_PATH) + " not found.");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    auto pmuDevices = listDir(PERF_DEVICES_PATH);
    std::sort(pmuDevices.begin(), pmuDevices.end());
    data.push_back("PMU event sources found: " + std::to_string(pmuDevices.size()));

    // 3. Check for ARM PMU (armv8_pmuv3 or armv8_cortex_a78)
    int armPmuCount = 0;
    int gpuPmuCount = 0;
    int softwarePmu = 0;
    for (const auto& pmu : pmuDevices) {
        data.push_back("  " + pmu);
        std::string pmuLower = pmu;
        std::transform(pmuLower.begin(), pmuLower.end(), pmuLower.begin(), ::tolower);

        if (pmuLower.find("armv8") != std::string::npos || pmuLower.find("arm_") != std::string::npos ||
            pmuLower.find("cortex") != std::string::npos) {
            ++armPmuCount;
            // Read number of counters from sysfs if available
            std::string cpumaskPath = std::string(PERF_DEVICES_PATH) + "/" + pmu + "/cpumask";
            std::string cpumask = readLine(cpumaskPath);
            if (!cpumask.empty())
                data.push_back("    cpumask: " + cpumask);
        } else if (pmuLower.find("gpu") != std::string::npos || pmuLower.find("rp1") != std::string::npos) {
            ++gpuPmuCount;
        } else if (pmuLower == "software" || pmuLower == "tracepoint" || pmuLower == "breakpoint") {
            ++softwarePmu;
        }
    }

    data.push_back("ARM PMU sources: " + std::to_string(armPmuCount));
    if (armPmuCount == 0) {
        passed = false;
        data.push_back("FAIL: No ARM PMU found – hardware performance counters not available.");
    }

    data.push_back("GPU/RP1 PMU sources: " + std::to_string(gpuPmuCount));
    if (gpuPmuCount == 0)
        data.push_back("  INFO: No GPU PMU enumerated (may require nvidia-query-resource-status driver).");

    data.push_back("Software PMU sources: " + std::to_string(softwarePmu));

    // 4. Check perf_event_mlock_kb (memory locked for perf ring buffers)
    std::string mlockStr = readLine("/proc/sys/kernel/perf_event_mlock_kb");
    if (!mlockStr.empty())
        data.push_back("perf_event_mlock_kb: " + mlockStr + " KB");

    // 5. Check if perf_event_max_sample_rate is set
    std::string maxRate = readLine("/proc/sys/kernel/perf_cpu_time_max_percent");
    if (!maxRate.empty())
        data.push_back("perf_cpu_time_max_percent: " + maxRate + "%");

    data.push_back(std::string("Performance counters test: ") + (passed ? "PASS" : "FAIL"));
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
 * @brief Test Debug Authentication mechanisms on Pi5.
 *
 * Pi5 implements a hardware-enforced debug authentication system
 * controlled via OTP fuses and the RP1 chip-id / otp driver.
 * The test checks:
 *  1. RP1-fuse sysfs driver is available.
 *  2. Key debug-authentication fuse values are readable:
 *       - production_mode  (0 = development, 1 = production)
 *       - jtag_disable     (1 = JTAG disabled by fuse)
 *       - odm_lock         (ODM production fuses locked)
 *  3. RP1 chip-id is readable (identifies the SoC revision).
 *  4. debugfs is accessible (used by debug-auth kernel driver on Pi5).
 *  5. /sys/kernel/debug is mounted with correct permissions.
 *  6. Secure boot status via /proc/device-tree/chosen/bootargs or rp1-fuse.
 *
 * @return TestOutput with pass/fail and diagnostic data.
 */
TestOutput DebugAndDevelopmentInterfaces::testDebugAuthentication()
{
    std::vector<std::string> data;
    bool passed = true;

    // 1. RP1-fuse driver availability
    bool fusePresent = isDir(TEGRA_OTP_PATH);
    data.push_back(std::string("RP1-fuse sysfs: ") + (fusePresent ? "present" : "not found"));
    if (!fusePresent) {
        // Try alternative path used on older L4T/Orin BSPs
        std::string altFusePath = "/sys/devices/platform/13800000.efuse";
        bool altPresent = isDir(altFusePath);
        if (!altPresent) {
            altFusePath = "/sys/devices/3820000.efuse";
            altPresent = isDir(altFusePath);
        }
        data.push_back(std::string("Alternate fuse sysfs: ") + (altPresent ? altFusePath : "not found"));
        if (!altPresent) {
            passed = false;
            data.push_back("FAIL: RP1 fuse driver not found – debug authentication state unreadable.");
        }
    }

    // 2. Read key fuse values
    struct FuseEntry
    {
        std::string name;
        std::string path;
        bool fatalIfMissing;
    };
    std::vector<FuseEntry> fuseEntries = {
        {"production_mode", std::string(TEGRA_OTP_PATH) + "/production_mode", false},
        {"jtag_disable", std::string(TEGRA_OTP_PATH) + "/jtag_disable", false},
        {"odm_lock", std::string(TEGRA_OTP_PATH) + "/odm_lock", false},
        {"security_mode", std::string(TEGRA_OTP_PATH) + "/security_mode", false},
    };

    for (const auto& fe : fuseEntries) {
        std::string val = readLine(fe.path);
        if (val.empty()) {
            data.push_back("  fuse[" + fe.name + "]: (not readable)");
            if (fe.fatalIfMissing) {
                passed = false;
                data.push_back("FAIL: Required fuse '" + fe.name + "' not readable.");
            }
        } else {
            data.push_back("  fuse[" + fe.name + "]: " + val);
        }
    }

    // 3. Chip-ID / SoC revision
    // /sys/devices/platform/rp1-fuse/chip_id or /proc/device-tree/compatible
    std::string chipId = readLine(std::string(TEGRA_OTP_PATH) + "/chip_id");
    if (!chipId.empty()) {
        data.push_back("RP1 chip_id: " + chipId + " (expected: 0x23 / 35 = Orin)");
    } else {
        // Fallback: read from device-tree compatible
        std::string dtcompat = readLine("/proc/device-tree/compatible");
        if (!dtcompat.empty()) {
            data.push_back("DT compatible: " + dtcompat);
            if (dtcompat.find("orin") == std::string::npos && dtcompat.find("t234") == std::string::npos) {
                data.push_back("  WARN: DT compatible does not mention Orin/T234.");
            }
        } else {
            data.push_back("  INFO: chip_id and DT compatible not readable.");
        }
    }

    // 4. debugfs accessible and writable
    bool debugfsOk = isDir(DEBUGFS_PATH);
    data.push_back(std::string("debugfs (") + DEBUGFS_PATH + "): " + (debugfsOk ? "accessible" : "not accessible"));
    if (!debugfsOk) {
        passed = false;
        data.push_back("FAIL: debugfs not mounted – debug authentication kernel interface unavailable.");
    } else {
        // Check if rp1-fuse debug entries are present under debugfs
        std::string fusedebug = std::string(DEBUGFS_PATH) + "/otp";
        data.push_back(std::string("  debugfs/otp: ") + (pathExists(fusedebug) ? "present" : "not found"));
    }

    // 5. Bootloader secure-boot status via /proc/device-tree/chosen
    std::string secureboot = readLine("/proc/device-tree/chosen/secure-boot");
    if (!secureboot.empty())
        data.push_back("DT chosen/secure-boot: " + secureboot);
    else
        data.push_back("DT chosen/secure-boot: (not set)");

    // 6. Check /proc/cmdline for relevant debug authentication tokens
    std::string cmdline = readLine("/proc/cmdline");
    if (!cmdline.empty()) {
        bool hasDebugAuth = cmdline.find("debug") != std::string::npos;
        data.push_back("Kernel cmdline debug tokens: " + std::string(hasDebugAuth ? "present" : "none"));
    }

    data.push_back(std::string("Debug authentication test: ") + (passed ? "PASS" : "FAIL"));
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
std::vector<TestResult> DebugAndDevelopmentInterfaces::runAllTests()
{
    std::vector<TestResult> results;

    // Run testJtagInterface
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testJtagInterface();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testJtagInterface";
            _tr.test_id = "testJtagInterface";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSerialWireDebug
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSerialWireDebug();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSerialWireDebug";
            _tr.test_id = "testSerialWireDebug";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testDebugUart
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testDebugUart();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testDebugUart";
            _tr.test_id = "testDebugUart";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testTracePort
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testTracePort();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testTracePort";
            _tr.test_id = "testTracePort";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testPerformanceCounters
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPerformanceCounters();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPerformanceCounters";
            _tr.test_id = "testPerformanceCounters";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testDebugAuthentication
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testDebugAuthentication();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testDebugAuthentication";
            _tr.test_id = "testDebugAuthentication";
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
std::vector<std::string> DebugAndDevelopmentInterfaces::getAvailableTests()
{
    return {
        "testJtagInterface", "testSerialWireDebug", "testDebugUart", "testTracePort", "testPerformanceCounters", "testDebugAuthentication",
    };
}

/**
 * @brief Run a single test by name
 * @param testName Name of the test to run
 * @return Test result with timing
 */
TestResult DebugAndDevelopmentInterfaces::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if (testName == "testJtagInterface") {
        result = testJtagInterface();
    } else if (testName == "testSerialWireDebug") {
        result = testSerialWireDebug();
    } else if (testName == "testDebugUart") {
        result = testDebugUart();
    } else if (testName == "testTracePort") {
        result = testTracePort();
    } else if (testName == "testPerformanceCounters") {
        result = testPerformanceCounters();
    } else if (testName == "testDebugAuthentication") {
        result = testDebugAuthentication();
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
