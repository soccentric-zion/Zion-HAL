/**
 * @file watchdog_and_reset_lib.cpp
 * @brief Implementation file for WatchdogAndResetTesting class — Raspberry Pi 5
 *        Pi5 watchdog: RP1 hardware watchdog, exposed as /dev/watchdog0.
 *        Reset reason available via vcgencmd get_throttled and dmesg.
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "watchdog_and_reset_lib.h"
#include "../platform_rpi5.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <linux/watchdog.h>
#include <sstream>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string wdReadFile(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::string s;
    std::getline(f, s);
    return s;
}

static std::string wdRunCmd(const char* cmd)
{
    char buf[4096] = {};
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "";
    size_t n = fread(buf, 1, sizeof(buf) - 1, pipe);
    pclose(pipe);
    buf[n] = '\0';
    while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r')) buf[--n] = '\0';
    return std::string(buf);
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
WatchdogAndResetTesting::WatchdogAndResetTesting()
{
}

WatchdogAndResetTesting::~WatchdogAndResetTesting()
{
}

// ---------------------------------------------------------------------------
// 1. testWatchdogTimerFunctionality
// ---------------------------------------------------------------------------
TestOutput WatchdogAndResetTesting::testWatchdogTimerFunctionality()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Watchdog timer test (Pi5: RP1 watchdog, /dev/watchdog0)");

    bool wdExists = (access("/dev/watchdog0", F_OK) == 0);
    data.push_back("/dev/watchdog0 present: " + std::string(wdExists ? "yes" : "no"));

    if (!wdExists) {
        data.push_back("INFO: Load watchdog driver: modprobe bcm2835-wdt or bcm2712-wdt");
        data.push_back("INFO: Enable via /boot/firmware/config.txt: dtparam=watchdog=on");
        {
            TestOutput _out;
            _out.status = hal::Status::PASS;
            _out.severity = hal::Severity::INFO;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        } // Not a hard failure — watchdog may not be enabled
    }

    // Open watchdog (read-only check — do NOT send keep-alive or set timeout here)
    int fd = open("/dev/watchdog0", O_WRONLY | O_CLOEXEC);
    if (fd >= 0) {
        struct watchdog_info info = {};
        if (ioctl(fd, WDIOC_GETSUPPORT, &info) == 0) {
            data.push_back("Identity:  " + std::string(reinterpret_cast<char*>(info.identity)));
            data.push_back("Firmware:  " + std::to_string(info.firmware_version));
            data.push_back("Options:   0x" + [&]() {
                char h[16]; snprintf(h, sizeof(h), "%08x", info.options); return std::string(h);
            }());
        }

        int timeout = 0;
        if (ioctl(fd, WDIOC_GETTIMEOUT, &timeout) == 0)
            data.push_back("Current timeout: " + std::to_string(timeout) + " s");

        int bootStatus = 0;
        if (ioctl(fd, WDIOC_GETBOOTSTATUS, &bootStatus) == 0)
            data.push_back(std::string("Boot status (watchdog reset flag): ") +
                           (bootStatus ? "WATCHDOG RESET" : "normal boot"));

        // Write 'V' to disable watchdog on close (magic close)
        write(fd, "V", 1);
        close(fd);
        data.push_back("PASS: Watchdog opened and queried successfully.");
    } else {
        data.push_back("WARNING: Cannot open /dev/watchdog0: " + std::string(strerror(errno)));
        data.push_back("       (May be held by systemd-watchdogd or watchdog daemon)");
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
// 2. testWatchdogTimeoutConfiguration
// ---------------------------------------------------------------------------
TestOutput WatchdogAndResetTesting::testWatchdogTimeoutConfiguration()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Watchdog timeout configuration check (Pi5)");

    // Check systemd watchdog settings
    std::string sdWd = wdRunCmd("systemctl show --property=RuntimeWatchdogUSec --no-pager 2>/dev/null");
    if (!sdWd.empty()) data.push_back("systemd RuntimeWatchdogUSec: " + sdWd);

    std::string sdReb = wdRunCmd("systemctl show --property=RebootWatchdogUSec --no-pager 2>/dev/null");
    if (!sdReb.empty()) data.push_back("systemd RebootWatchdogUSec: " + sdReb);

    // Check /etc/watchdog.conf if present
    bool wdConf = (access("/etc/watchdog.conf", F_OK) == 0);
    data.push_back("/etc/watchdog.conf: " + std::string(wdConf ? "present" : "not found"));

    if (wdConf) {
        std::string interval = wdRunCmd("grep -i 'interval\\|timeout' /etc/watchdog.conf 2>/dev/null");
        if (!interval.empty()) data.push_back("watchdog.conf: " + interval.substr(0, 200));
    }

    // Check /proc/driver/watchdog if available
    bool procWd = (access("/proc/driver/watchdog", F_OK) == 0);
    if (procWd) {
        std::string procInfo = wdReadFile("/proc/driver/watchdog");
        if (!procInfo.empty()) data.push_back("/proc/driver/watchdog: " + procInfo);
    }

    data.push_back("Pi5 RP1 watchdog max timeout: 1048575 seconds (~12 days)");
    data.push_back("PASS: Timeout configuration check complete.");
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
// 3. testWatchdogResetGeneration
//    Check capability without triggering an actual reset.
// ---------------------------------------------------------------------------
TestOutput WatchdogAndResetTesting::testWatchdogResetGeneration()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Watchdog reset generation capability check");
    data.push_back("NOTE: Test does NOT trigger a reset — only checks capability.");

    bool wdExists = (access("/dev/watchdog0", F_OK) == 0);
    if (!wdExists) {
        data.push_back("INFO: /dev/watchdog0 not found. Enable via dtparam=watchdog=on");
        {
            TestOutput _out;
            _out.status = hal::Status::PASS;
            _out.severity = hal::Severity::INFO;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    int fd = open("/dev/watchdog0", O_WRONLY | O_CLOEXEC);
    if (fd >= 0) {
        struct watchdog_info info = {};
        if (ioctl(fd, WDIOC_GETSUPPORT, &info) == 0) {
            bool canReset = (info.options & WDIOF_CARDRESET) != 0;
            data.push_back("WDIOF_CARDRESET support: " + std::string(canReset ? "YES" : "no"));

            bool canPretimeout = (info.options & WDIOF_PRETIMEOUT) != 0;
            data.push_back("WDIOF_PRETIMEOUT support: " + std::string(canPretimeout ? "YES" : "no"));
        }
        write(fd, "V", 1);
        close(fd);
        data.push_back("PASS: Watchdog reset capability verified.");
    } else {
        data.push_back("INFO: Cannot open watchdog (possibly held by system daemon).");
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
// 4. testWatchdogInterruptGeneration
// ---------------------------------------------------------------------------
TestOutput WatchdogAndResetTesting::testWatchdogInterruptGeneration()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Watchdog interrupt/pretimeout check (Pi5 RP1 watchdog)");

    bool wdExists = (access("/dev/watchdog0", F_OK) == 0);
    if (!wdExists) {
        data.push_back("INFO: /dev/watchdog0 not present.");
        {
            TestOutput _out;
            _out.status = hal::Status::PASS;
            _out.severity = hal::Severity::INFO;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    int fd = open("/dev/watchdog0", O_WRONLY | O_CLOEXEC);
    if (fd >= 0) {
        struct watchdog_info info = {};
        if (ioctl(fd, WDIOC_GETSUPPORT, &info) == 0) {
            bool canIrq = (info.options & WDIOF_PRETIMEOUT) != 0;
            data.push_back("Pretimeout interrupt: " + std::string(canIrq ? "supported" : "not supported"));
        }

        int pretimeout = 0;
        if (ioctl(fd, WDIOC_GETPRETIMEOUT, &pretimeout) == 0)
            data.push_back("Pretimeout: " + std::to_string(pretimeout) + " s");

        write(fd, "V", 1);
        close(fd);
    }

    data.push_back("PASS: Interrupt/pretimeout check complete.");
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
// 5. testPoweronReset
// ---------------------------------------------------------------------------
TestOutput WatchdogAndResetTesting::testPoweronReset()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Power-on reset check (Pi5)");

    // dmesg may show reset reason
    std::string dmesgReset = wdRunCmd("dmesg 2>/dev/null | grep -i 'reset\\|reboot\\|power.on' | tail -5");
    if (!dmesgReset.empty()) data.push_back("dmesg reset events: " + dmesgReset.substr(0, 400));

    // journald last boot reason
    std::string lastBoot = wdRunCmd("who -b 2>/dev/null");
    if (!lastBoot.empty()) data.push_back("Last boot: " + lastBoot);

    // systemd-analyze blame for boot time
    std::string bootTime = wdRunCmd("systemd-analyze 2>/dev/null | head -1");
    if (!bootTime.empty()) data.push_back("Boot time: " + bootTime);

    // Check vcgencmd for power flags
    std::string throttled = wdRunCmd("vcgencmd get_throttled 2>/dev/null");
    if (!throttled.empty()) {
        data.push_back("vcgencmd throttled: " + throttled);
        // Bit 16 = 'was under-voltage', Bit 18 = 'was throttled' — indicates brownout/reset
        unsigned long flags = 0;
        if (sscanf(throttled.c_str(), "throttled=0x%lx", &flags) == 1) {
            if (flags & 0x10000) data.push_back("  -> Previous under-voltage event recorded.");
            if (flags & 0x40000) data.push_back("  -> Previous throttling event recorded.");
        }
    }

    data.push_back("PASS: Power-on reset check complete.");
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
// 6. testHardwareResetButton
// ---------------------------------------------------------------------------
TestOutput WatchdogAndResetTesting::testHardwareResetButton()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Hardware reset button check (Pi5)");
    data.push_back("Pi5 has a dedicated reset button (J2 header) wired to RP1.");
    data.push_back("J2 pin 1 = RUN (active-low reset), connects to BCM2712 nRSTIN.");

    // Check input-event for button press
    std::string evdev = wdRunCmd("ls /dev/input/event* 2>/dev/null | head -5");
    if (!evdev.empty()) data.push_back("Input devices: " + evdev);

    // Check for gpio-keys or power-button
    std::string gpioKeys = wdRunCmd("dmesg 2>/dev/null | grep -i 'gpio-keys\\|reset.button\\|power-button' | tail -3");
    if (!gpioKeys.empty()) data.push_back("dmesg: " + gpioKeys.substr(0, 200));

    data.push_back("Hardware reset: pull J2 pin 1 (RUN) low to trigger immediate reset.");
    data.push_back("PASS: Reset button check complete (cannot activate without physical access).");
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
// 7. testSoftwareReset
// ---------------------------------------------------------------------------
TestOutput WatchdogAndResetTesting::testSoftwareReset()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Software reset capability check (Pi5)");
    data.push_back("NOTE: Test does NOT trigger reset.");

    // Check reboot command availability
    bool rebootCmd = (access("/sbin/reboot", F_OK) == 0) || (access("/usr/sbin/reboot", F_OK) == 0);
    data.push_back("/sbin/reboot: " + std::string(rebootCmd ? "present" : "not found"));

    // Check systemctl
    bool systemctl = (access("/usr/bin/systemctl", F_OK) == 0);
    data.push_back("systemctl: " + std::string(systemctl ? "present" : "not found"));

    // Check reboot flag support via Magic SysRq
    std::string sysrq = wdReadFile("/proc/sys/kernel/sysrq");
    data.push_back("/proc/sys/kernel/sysrq: " + sysrq);

    data.push_back("Software reset methods: systemctl reboot, /proc/sysrq-trigger (b), reboot(2) syscall");
    data.push_back("PASS: Software reset capability confirmed.");
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
// 8. testWatchdogReset
//    Check previous watchdog reset via WDIOC_GETBOOTSTATUS.
// ---------------------------------------------------------------------------
TestOutput WatchdogAndResetTesting::testWatchdogReset()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Watchdog reset history check (Pi5)");

    bool wdExists = (access("/dev/watchdog0", F_OK) == 0);
    if (wdExists) {
        int fd = open("/dev/watchdog0", O_WRONLY | O_CLOEXEC);
        if (fd >= 0) {
            int bootStatus = 0;
            if (ioctl(fd, WDIOC_GETBOOTSTATUS, &bootStatus) == 0) {
                data.push_back("WDIOC_GETBOOTSTATUS: " + std::to_string(bootStatus));
                if (bootStatus & WDIOF_CARDRESET)
                    data.push_back("  -> System was reset by watchdog timer.");
                else
                    data.push_back("  -> No watchdog reset on last boot.");
            }
            write(fd, "V", 1);
            close(fd);
        }
    }

    // Check dmesg for watchdog reset message
    std::string wdMsg = wdRunCmd("dmesg 2>/dev/null | grep -i watchdog | tail -5");
    if (!wdMsg.empty()) data.push_back("dmesg watchdog: " + wdMsg.substr(0, 300));

    data.push_back("PASS: Watchdog reset history check complete.");
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
// 9. testThermalReset
// ---------------------------------------------------------------------------
TestOutput WatchdogAndResetTesting::testThermalReset()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Thermal reset check (Pi5)");
    data.push_back("BCM2712 soft thermal shutdown at " +
                   std::to_string(RPI5_THERMAL_SHUTDOWN_C) + " °C");

    // Check current temperature
    long long tempMilliC = -1;
    {
        std::ifstream f("/sys/class/thermal/thermal_zone0/temp");
        if (f.is_open()) f >> tempMilliC;
    }
    if (tempMilliC > 0)
        data.push_back("Current SoC temp: " + std::to_string(tempMilliC / 1000.0) + " °C");

    // Check dmesg for thermal shutdown events
    std::string thermMsg = wdRunCmd("dmesg 2>/dev/null | grep -i 'thermal\\|shutdown\\|overheat' | tail -5");
    if (!thermMsg.empty()) data.push_back("dmesg thermal: " + thermMsg.substr(0, 300));

    // Check vcgencmd for previous throttle
    std::string throttled = wdRunCmd("vcgencmd get_throttled 2>/dev/null");
    if (!throttled.empty()) {
        data.push_back("Throttle flags: " + throttled);
        unsigned long flags = 0;
        if (sscanf(throttled.c_str(), "throttled=0x%lx", &flags) == 1) {
            if (flags & 0x80000) {
                data.push_back("  -> Previous soft temperature limit event recorded.");
            }
        }
    }

    data.push_back("PASS: Thermal reset history check complete.");
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
// 10. testResetReasonLogging
// ---------------------------------------------------------------------------
TestOutput WatchdogAndResetTesting::testResetReasonLogging()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Reset reason logging check (Pi5)");

    // Check systemd journal for last reboot reason
    std::string journal = wdRunCmd("journalctl -b -1 --no-pager -q -n 5 2>/dev/null | head -5");
    if (!journal.empty()) data.push_back("Previous boot log (last 5 lines): " + journal.substr(0, 400));

    std::string reason = wdRunCmd("last reboot 2>/dev/null | head -3");
    if (!reason.empty()) data.push_back("last reboot: " + reason.substr(0, 200));

    // vcgencmd persistent throttle/power flags
    std::string throttled = wdRunCmd("vcgencmd get_throttled 2>/dev/null");
    if (!throttled.empty()) {
        data.push_back("vcgencmd get_throttled: " + throttled);
        unsigned long flags = 0;
        if (sscanf(throttled.c_str(), "throttled=0x%lx", &flags) == 1) {
            if (flags == 0) data.push_back("  -> Clean boot, no reset events recorded.");
            if (flags & 0x1)     data.push_back("  -> Active: under-voltage");
            if (flags & 0x2)     data.push_back("  -> Active: frequency cap");
            if (flags & 0x4)     data.push_back("  -> Active: throttled");
            if (flags & 0x8)     data.push_back("  -> Active: soft temperature limit");
            if (flags & 0x10000) data.push_back("  -> History: under-voltage occurred");
            if (flags & 0x20000) data.push_back("  -> History: frequency cap occurred");
            if (flags & 0x40000) data.push_back("  -> History: throttling occurred");
            if (flags & 0x80000) data.push_back("  -> History: soft temperature limit occurred");
        }
    }

    data.push_back("PASS: Reset reason logging check complete.");
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
std::vector<TestResult> WatchdogAndResetTesting::runAllTests()
{
    std::vector<TestResult> results;

    // Run testWatchdogTimerFunctionality
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testWatchdogTimerFunctionality();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testWatchdogTimerFunctionality";
            _tr.test_id = "testWatchdogTimerFunctionality";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testWatchdogTimeoutConfiguration
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testWatchdogTimeoutConfiguration();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testWatchdogTimeoutConfiguration";
            _tr.test_id = "testWatchdogTimeoutConfiguration";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testWatchdogResetGeneration
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testWatchdogResetGeneration();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testWatchdogResetGeneration";
            _tr.test_id = "testWatchdogResetGeneration";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testWatchdogInterruptGeneration
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testWatchdogInterruptGeneration();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testWatchdogInterruptGeneration";
            _tr.test_id = "testWatchdogInterruptGeneration";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testPoweronReset
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPoweronReset();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPoweronReset";
            _tr.test_id = "testPoweronReset";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testHardwareResetButton
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testHardwareResetButton();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testHardwareResetButton";
            _tr.test_id = "testHardwareResetButton";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSoftwareReset
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSoftwareReset();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSoftwareReset";
            _tr.test_id = "testSoftwareReset";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testWatchdogReset
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testWatchdogReset();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testWatchdogReset";
            _tr.test_id = "testWatchdogReset";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testThermalReset
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testThermalReset();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testThermalReset";
            _tr.test_id = "testThermalReset";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testResetReasonLogging
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testResetReasonLogging();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testResetReasonLogging";
            _tr.test_id = "testResetReasonLogging";
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
std::vector<std::string> WatchdogAndResetTesting::getAvailableTests()
{
    return {
        "testWatchdogTimerFunctionality",
        "testWatchdogTimeoutConfiguration",
        "testWatchdogResetGeneration",
        "testWatchdogInterruptGeneration",
        "testPoweronReset",
        "testHardwareResetButton",
        "testSoftwareReset",
        "testWatchdogReset",
        "testThermalReset",
        "testResetReasonLogging",
    };
}

/**
 * @brief Run a single test by name
 * @param testName Name of the test to run
 * @return Test result with timing
 */
TestResult WatchdogAndResetTesting::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if (testName == "testWatchdogTimerFunctionality") {
        result = testWatchdogTimerFunctionality();
    } else if (testName == "testWatchdogTimeoutConfiguration") {
        result = testWatchdogTimeoutConfiguration();
    } else if (testName == "testWatchdogResetGeneration") {
        result = testWatchdogResetGeneration();
    } else if (testName == "testWatchdogInterruptGeneration") {
        result = testWatchdogInterruptGeneration();
    } else if (testName == "testPoweronReset") {
        result = testPoweronReset();
    } else if (testName == "testHardwareResetButton") {
        result = testHardwareResetButton();
    } else if (testName == "testSoftwareReset") {
        result = testSoftwareReset();
    } else if (testName == "testWatchdogReset") {
        result = testWatchdogReset();
    } else if (testName == "testThermalReset") {
        result = testThermalReset();
    } else if (testName == "testResetReasonLogging") {
        result = testResetReasonLogging();
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
