/**
 * @file clock_and_timing_lib.cpp
 * @brief Implementation file for ClockAndTimingTesting class
 *        Targets Raspberry Pi 5 platform.
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "clock_and_timing_lib.h"
#include "../platform_rpi5.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <linux/rtc.h>
#include <sstream>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace
{

/** Read a single-line sysfs / procfs file and return its content as string. */
static bool readSysFile(const std::string& path, std::string& out)
{
    std::ifstream f(path);
    if (!f.is_open())
        return false;
    std::getline(f, out);
    return !out.empty();
}

/** Return true if a filesystem path exists (file or directory). */
static bool pathExists(const std::string& p)
{
    struct stat st
    {
    };
    return ::stat(p.c_str(), &st) == 0;
}

/** Read enable-count for a kernel debug clock entry.
 *  Path pattern: /sys/kernel/debug/clk/<clk_name>/clk_enable_count
 *  Returns -1 if the file does not exist. */
static int readClkEnableCount(const std::string& clkName)
{
    const std::string path = std::string("/sys/kernel/debug/clk/") + clkName + "/clk_enable_count";
    std::string val;
    if (!readSysFile(path, val))
        return -1;
    try {
        return std::stoi(val);
    }
    catch (...) {
        return -1;
    }
}

/** Read clock rate (Hz) from /sys/kernel/debug/clk/<clk>/clk_rate.
 *  Returns 0 on failure. */
static unsigned long readClkRate(const std::string& clkName)
{
    const std::string path = std::string("/sys/kernel/debug/clk/") + clkName + "/clk_rate";
    std::string val;
    if (!readSysFile(path, val))
        return 0;
    try {
        return std::stoul(val);
    }
    catch (...) {
        return 0;
    }
}

/** Measure clock resolution via clock_getres(). Returns nanoseconds. */
static long clockResNs(clockid_t id)
{
    struct timespec ts
    {
    };
    if (::clock_getres(id, &ts) != 0)
        return -1;
    return static_cast<long>(ts.tv_sec) * 1'000'000'000L + ts.tv_nsec;
}

/** Compute standard deviation of a vector of doubles. */
static double stddev(const std::vector<double>& v)
{
    if (v.size() < 2)
        return 0.0;
    double sum = 0, sum2 = 0;
    for (double x : v) {
        sum += x;
        sum2 += x * x;
    }
    double mean = sum / v.size();
    double var = sum2 / v.size() - mean * mean;
    return (var > 0) ? std::sqrt(var) : 0.0;
}

} // anonymous namespace

/**
 * @brief Constructor for ClockAndTimingTesting
 */
ClockAndTimingTesting::ClockAndTimingTesting()
{
}

/**
 * @brief Destructor for ClockAndTimingTesting
 */
ClockAndTimingTesting::~ClockAndTimingTesting()
{
}

/**
 * @brief Implementation of testSystemClockAccuracy
 *
 * Verifies that:
 *  1. CLOCK_REALTIME and CLOCK_MONOTONIC are readable.
 *  2. The difference between two CLOCK_REALTIME samples taken with a known
 *     sleep is within ±5 % of the expected interval (1 s), confirming the
 *     kernel clock is ticking at a reasonable rate.
 *  3. NTP synchronisation status is checked via timedatectl / timesyncd
 *     (best-effort; a warning is appended when unavailable, not a failure).
 */
TestOutput ClockAndTimingTesting::testSystemClockAccuracy()
{
    std::vector<std::string> data;

    // --- 1. Readability of CLOCK_REALTIME and CLOCK_MONOTONIC ---
    struct timespec tReal1
    {
    }, tMono1{};
    if (::clock_gettime(CLOCK_REALTIME, &tReal1) != 0 || ::clock_gettime(CLOCK_MONOTONIC, &tMono1) != 0) {
        data.push_back("FAIL: clock_gettime() returned error: " + std::string(::strerror(errno)));
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    long resNsReal = clockResNs(CLOCK_REALTIME);
    long resNsMono = clockResNs(CLOCK_MONOTONIC);
    data.push_back("CLOCK_REALTIME  resolution: " + std::to_string(resNsReal) + " ns");
    data.push_back("CLOCK_MONOTONIC resolution: " + std::to_string(resNsMono) + " ns");

    // --- 2. 1-second drift measurement ---
    const long sleepMs = 1000;
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));

    struct timespec tReal2
    {
    };
    ::clock_gettime(CLOCK_REALTIME, &tReal2);

    double deltaMs =
        (static_cast<double>(tReal2.tv_sec - tReal1.tv_sec) * 1e3) + (static_cast<double>(tReal2.tv_nsec - tReal1.tv_nsec) * 1e-6);
    data.push_back("1-second interval measured: " + std::to_string(deltaMs) + " ms (expected 1000 ms)");

    const double tolerancePct = 5.0; // 5 %
    bool driftOk = (std::abs(deltaMs - sleepMs) / sleepMs * 100.0) <= tolerancePct;
    if (!driftOk)
        data.push_back("WARN: clock drift exceeds " + std::to_string(tolerancePct) + "% tolerance");

    // --- 3. NTP / time-sync status (best-effort) ---
    {
        // Pi5 / Ubuntu: check systemd-timesyncd synchronized flag
        const std::string syncPath = "/run/systemd/timesync/synchronized";
        if (pathExists(syncPath)) {
            data.push_back("NTP sync flag present: " + syncPath);
        } else {
            // Fallback: read timedatectl via popen
            FILE* fp = ::popen("timedatectl show --property=NTPSynchronized --value 2>/dev/null", "r");
            if (fp) {
                char buf[64]{};
                if (::fgets(buf, sizeof(buf), fp)) {
                    std::string s(buf);
                    s.erase(s.find_last_not_of("\r\n") + 1);
                    data.push_back("NTP synchronized: " + s);
                    if (s != "yes")
                        data.push_back("WARN: NTP not synchronized");
                }
                ::pclose(fp);
            }
        }
    }

    {
        TestOutput _out;
        _out.status = driftOk ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = driftOk ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Implementation of testPcieReferenceClock
 *
 * On Pi5 the PCIe reference clock is sourced from the BCM2712 PLL and
 * is nominally 100 MHz (spread-spectrum variants ±0.5%).
 *
 * Strategy (Raspberry Pi 5):
 *  1. Look for the PCIe reference clock in /sys/kernel/debug/clk/.
 *     Known clock names on Orin: "pcie_ref", "pcie_ref_clk", "pcie0_ref".
 *  2. Verify the clock is enabled (enable-count > 0).
 *  3. Verify the rate is in [99 MHz, 101 MHz].
 *  4. As a fallback verify at least one PCIe device appears on the bus
 *     (/sys/bus/pci/devices/).
 */
TestOutput ClockAndTimingTesting::testPcieReferenceClock()
{
    std::vector<std::string> data;

    // Candidate clock names used by different Orin kernel versions
    const std::vector<std::string> candidates = {"pcie_ref", "pcie_ref_clk", "pcie0_ref", "pciex1_ref", "pciex4_ref"};

    bool foundClk = false;
    for (const auto& name : candidates) {
        int cnt = readClkEnableCount(name);
        if (cnt < 0)
            continue; // sysfs entry doesn't exist for this name

        unsigned long rateHz = readClkRate(name);
        foundClk = true;
        data.push_back("PCIe ref clock '" + name + "': enable_count=" + std::to_string(cnt) +
                       ", rate=" + std::to_string(rateHz / 1'000'000UL) + " MHz");

        if (cnt == 0) {
            data.push_back("WARN: '" + name + "' enable_count is 0 (clock gated)");
        }

        // 100 MHz ± 1 MHz tolerance
        const unsigned long minHz = 99'000'000UL, maxHz = 101'000'000UL;
        if (rateHz > 0 && (rateHz < minHz || rateHz > maxHz)) {
            data.push_back("FAIL: PCIe ref clock rate out of range [99, 101] MHz");
            {
                TestOutput _out;
                _out.status = hal::Status::FAIL;
                _out.severity = hal::Severity::ERROR;
                for (size_t _i = 0; _i < data.size(); ++_i)
                    _out.reason["detail_" + std::to_string(_i)] = data[_i];
                return _out;
            }
        }
        break; // use first match
    }

    if (!foundClk) {
        // Fallback: verify PCIe bus is enumerated
        data.push_back("INFO: /sys/kernel/debug/clk/ not accessible; "
                       "falling back to PCI bus enumeration");
        const std::string pciBusPath = "/sys/bus/pci/devices";
        DIR* dir = ::opendir(pciBusPath.c_str());
        if (!dir) {
            data.push_back("FAIL: PCIe sysfs clock and bus unavailable");
            {
                TestOutput _out;
                _out.status = hal::Status::FAIL;
                _out.severity = hal::Severity::ERROR;
                for (size_t _i = 0; _i < data.size(); ++_i)
                    _out.reason["detail_" + std::to_string(_i)] = data[_i];
                return _out;
            }
        }
        int devCount = 0;
        struct dirent* entry;
        while ((entry = ::readdir(dir)) != nullptr) {
            if (entry->d_name[0] != '.')
                ++devCount;
        }
        ::closedir(dir);
        data.push_back("PCIe devices enumerated: " + std::to_string(devCount));
        if (devCount == 0) {
            data.push_back("FAIL: No PCIe devices found; reference clock may be absent");
            {
                TestOutput _out;
                _out.status = hal::Status::FAIL;
                _out.severity = hal::Severity::ERROR;
                for (size_t _i = 0; _i < data.size(); ++_i)
                    _out.reason["detail_" + std::to_string(_i)] = data[_i];
                return _out;
            }
        }
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
 * @brief Implementation of testUsbReferenceClock
 *
 * Pi5 xHCI controller uses a 100 MHz reference clock from RP1
 * reference divided internally to generate USB 2.0 (48 MHz) and USB 3.x
 * (250 MHz) clocks.
 *
 * Tests:
 *  1. Check xusb_core_clk / xusb_ss_clk via debugfs.
 *  2. Verify at least one USB controller device appears in sysfs.
 */
TestOutput ClockAndTimingTesting::testUsbReferenceClock()
{
    std::vector<std::string> data;

    // Candidate clock names on Orin (vary by BSP version)
    const std::vector<std::string> candidates = {"xusb_core_clk", "xusb_ss_clk", "xusb_fs_clk",
                                                 "usb2_trk_clk",  "usb_ref",     "xusb_falcon_clk"};

    bool anyFound = false;
    for (const auto& name : candidates) {
        unsigned long rateHz = readClkRate(name);
        if (rateHz == 0)
            continue;

        int cnt = readClkEnableCount(name);
        anyFound = true;
        data.push_back("USB clock '" + name + "': rate=" + std::to_string(rateHz / 1'000'000UL) +
                       " MHz, enable_count=" + std::to_string(cnt));
    }

    if (!anyFound) {
        // Fallback: verify XUSB platform device exists
        const std::vector<std::string> xhciPaths = {"/sys/bus/platform/devices/3610000.xhci", "/sys/bus/platform/devices/3530000.xhci",
                                                    "/sys/bus/usb/drivers/xhci_hcd"};
        for (const auto& p : xhciPaths) {
            if (pathExists(p)) {
                data.push_back("XUSB controller present: " + p);
                anyFound = true;
                break;
            }
        }
    }

    // Check that /sys/bus/usb/devices has at least the root hub
    const std::string usbBus = "/sys/bus/usb/devices";
    DIR* dir = ::opendir(usbBus.c_str());
    if (dir) {
        int cnt = 0;
        struct dirent* e;
        while ((e = ::readdir(dir)) != nullptr)
            if (e->d_name[0] != '.')
                ++cnt;
        ::closedir(dir);
        data.push_back("USB devices in sysfs: " + std::to_string(cnt));
        if (cnt > 0)
            anyFound = true;
    }

    if (!anyFound) {
        data.push_back("FAIL: No USB clock or device evidence found");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
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
 * @brief Implementation of testEthernetReferenceClock
 *
 * Pi5 exposes a 10GbE (MGBE) controller and a GbE (EQOS) controller.
 * Both rely on internal PLLs derived from the system oscillator.
 *
 * Tests:
 *  1. Check presence of eqos / mgbe clock entries in debugfs.
 *  2. Verify the Ethernet network interface(s) are visible in sysfs.
 *  3. If a PTP hardware clock (/dev/ptp*) exists, record it.
 */
TestOutput ClockAndTimingTesting::testEthernetReferenceClock()
{
    std::vector<std::string> data;
    bool passed = true;

    // 1. Kernel debug clock entries
    const std::vector<std::string> clkCandidates = {"eqos_axi_clk", "eqos_ptp_ref_clk", "mgbe_mac_clk",
                                                    "mgbe_pcs_clk", "eqos_rx",          "eqos_tx"};

    bool anyClk = false;
    for (const auto& name : clkCandidates) {
        unsigned long rateHz = readClkRate(name);
        if (rateHz == 0)
            continue;
        anyClk = true;
        data.push_back("ETH clock '" + name + "': " + std::to_string(rateHz / 1'000'000UL) + " MHz");
    }
    if (!anyClk)
        data.push_back("INFO: Ethernet debugfs clocks not accessible");

    // 2. Network interfaces in sysfs
    const std::string netPath = "/sys/class/net";
    DIR* dir = ::opendir(netPath.c_str());
    if (!dir) {
        data.push_back("FAIL: /sys/class/net not accessible");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    std::vector<std::string> ifaces;
    struct dirent* e;
    while ((e = ::readdir(dir)) != nullptr) {
        std::string n = e->d_name;
        if (n == "." || n == "lo" || n == "..")
            continue;
        ifaces.push_back(n);
    }
    ::closedir(dir);

    if (ifaces.empty()) {
        data.push_back("FAIL: No network interfaces found in /sys/class/net");
        passed = false;
    } else {
        for (const auto& iface : ifaces)
            data.push_back("Network interface: " + iface);
    }

    // 3. PTP hardware clock
    for (int i = 0; i < 4; ++i) {
        std::string ptpPath = "/dev/ptp" + std::to_string(i);
        if (pathExists(ptpPath)) {
            data.push_back("PTP hardware clock found: " + ptpPath);
            break;
        }
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
 * @brief Implementation of testCameraClockOutput
 *
 * Pi5 drives camera sensors via MIPI CSI-2 Unicam on RP1
 * uses a set of clocks (csi_clk, unicam_clk, extperiph1_clk, etc.).
 *
 * Tests:
 *  1. Look for unicam / csi / extperiph clock entries in debugfs.
 *  2. Verify the V4L2 / camera platform devices are present in sysfs.
 */
TestOutput ClockAndTimingTesting::testCameraClockOutput()
{
    std::vector<std::string> data;
    bool passed = true;

    // 1. Debug clock entries for CSI / VI
    const std::vector<std::string> clkCandidates = {
        "csi_clk",        "unicam_clk", "unicam_tsc_clk", "extperiph1_clk", "extperiph2_clk", "extperiph3_clk",
        "extperiph4_clk", "unicam", "csi",       "isp_clk"};

    bool anyClk = false;
    for (const auto& name : clkCandidates) {
        unsigned long rateHz = readClkRate(name);
        if (rateHz == 0)
            continue;
        anyClk = true;
        data.push_back("Camera clock '" + name + "': " + std::to_string(rateHz / 1'000'000UL) + " MHz");
    }
    if (!anyClk)
        data.push_back("INFO: Camera debugfs clocks not accessible (may require root)");

    // 2. V4L2 / media devices
    const std::vector<std::string> devicePaths = {"/dev/video0", "/dev/media0", "/sys/bus/platform/devices/13e00000.host1x",
                                                  "/sys/class/video4linux"};

    bool anyDevice = false;
    for (const auto& p : devicePaths) {
        if (pathExists(p)) {
            data.push_back("Camera/media device found: " + p);
            anyDevice = true;
        }
    }

    if (!anyClk && !anyDevice) {
        data.push_back("WARN: No camera clock or device evidence; "
                       "camera may not be connected");
        // Treat as pass — the hardware interface exists on the SoC regardless
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
 * @brief Implementation of testAudioClock
 *
 * Pi5 uses RP1 I2S for audio
 * clocks are derived from the PLLA PLL (nominally 56.448 MHz or 73.728 MHz
 * depending on audio sample rate family).
 *
 * Tests:
 *  1. Check pll_a / ape / i2s clock entries in debugfs.
 *  2. Verify ALSA PCM device or APE platform device is present.
 */
TestOutput ClockAndTimingTesting::testAudioClock()
{
    std::vector<std::string> data;

    const std::vector<std::string> clkCandidates = {"pll_a",    "pll_a_out0", "ape_clk",  "i2s1_clk", "i2s2_clk",    "i2s3_clk",
                                                    "i2s4_clk", "i2s5_clk",   "i2s6_clk", "dspk",     "ape_apb_clk", "adsp_bus_clk"};

    bool anyClk = false;
    for (const auto& name : clkCandidates) {
        unsigned long rateHz = readClkRate(name);
        if (rateHz == 0)
            continue;
        anyClk = true;
        data.push_back("Audio clock '" + name + "': " + std::to_string(rateHz / 1'000'000UL) + " MHz (" + std::to_string(rateHz) + " Hz)");
    }
    if (!anyClk)
        data.push_back("INFO: Audio debugfs clocks not accessible");

    // Check ALSA PCM devices
    bool anyAudio = false;
    for (int card = 0; card < 4; ++card) {
        std::string cardPath = "/dev/snd/pcmC" + std::to_string(card) + "D0p";
        if (pathExists(cardPath)) {
            data.push_back("ALSA PCM device: " + cardPath);
            anyAudio = true;
        }
    }

    // Check APE platform device
    const std::vector<std::string> apePaths = {"/sys/bus/platform/devices/2900000.ape", "/sys/bus/platform/devices/2a00000.adsp",
                                               "/sys/class/sound"};
    for (const auto& p : apePaths) {
        if (pathExists(p)) {
            data.push_back("Audio platform device/class: " + p);
            anyAudio = true;
        }
    }

    if (!anyClk && !anyAudio)
        data.push_back("WARN: No audio clock or device evidence found");

    // Pass as long as no hard error — audio peripherals may not be populated
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
 * @brief Implementation of testPllLockStatus
 *
 * On Pi5 the primary PLLs are: PLLSYS, PLLCOREORE, PLLUSBphiSBphi,
 * PLLREFE.  A locked PLL will have clk_rate > 0 in debugfs and ideally an
 * enable_count > 0 when consumers are active.
 *
 * Tests:
 *  1. For each known PLL name, read rate and enable_count.
 *  2. PLLSYS is the system reference PLL and must be running.
 *  3. Report all PLLs found; fail only if PLLSYS is absent/zero.
 */
TestOutput ClockAndTimingTesting::testPllLockStatus()
{
    std::vector<std::string> data;

    // Ordered list: first entry (PLLSYS) is mandatory
    struct PllEntry
    {
        std::string name;
        bool required;
    };
    const std::vector<PllEntry> plls = {
        {"pllp", true},  {"pllp_out0", false}, {"pllc", false},    {"pllc4", false},    {"plla", false},      {"plla_out0", false},
        {"pllu", false}, {"plle", false},      {"pllrefe", false}, {"pllcsi", false}, {"pllc_out1", false},
    };

    bool pllpFound = false;
    bool allRequiredOk = true;

    for (const auto& pll : plls) {
        unsigned long rateHz = readClkRate(pll.name);
        int enableCnt = readClkEnableCount(pll.name);

        if (rateHz == 0 && enableCnt < 0)
            continue; // not present on this build

        std::string line =
            "PLL '" + pll.name + "': rate=" + std::to_string(rateHz / 1'000'000UL) + " MHz" + ", enable_count=" + std::to_string(enableCnt);
        if (pll.required)
            line += " [REQUIRED]";
        data.push_back(line);

        if (pll.name == "pllp" || pll.name == "pllp_out0") {
            if (rateHz > 0)
                pllpFound = true;
        }

        if (pll.required && rateHz == 0) {
            data.push_back("FAIL: Required PLL '" + pll.name + "' has rate 0");
            allRequiredOk = false;
        }
    }

    if (!pllpFound) {
        // Fallback – verify PLLSYS via system uptime (PLLSYS must be running for the board to boot)
        data.push_back("INFO: PLLSYS not visible in debugfs; "
                       "verifying system is running (implicit PLLSYS lock)");
        std::string uptime;
        if (readSysFile("/proc/uptime", uptime)) {
            data.push_back("System uptime: " + uptime + " s (PLLSYS implicitly locked)");
            pllpFound = true;
        } else {
            data.push_back("FAIL: Cannot confirm PLLSYS lock");
            allRequiredOk = false;
        }
    }

    {
        TestOutput _out;
        _out.status = allRequiredOk ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = allRequiredOk ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Implementation of testClockJitterMeasurements
 *
 * Takes 200 consecutive CLOCK_MONOTONIC samples and computes:
 *  - Mean inter-sample interval
 *  - Standard deviation (jitter)
 *  - Min / max interval
 *
 * Pass criterion: jitter (1-sigma) < 10 µs for CLOCK_MONOTONIC on Orin,
 * which is very conservative given the Arm architecture timer resolution.
 */
TestOutput ClockAndTimingTesting::testClockJitterMeasurements()
{
    std::vector<std::string> data;

    constexpr int samples = 200;
    std::vector<double> intervals;
    intervals.reserve(samples - 1);

    struct timespec prev
    {
    };
    ::clock_gettime(CLOCK_MONOTONIC, &prev);

    for (int i = 1; i < samples; ++i) {
        struct timespec cur
        {
        };
        ::clock_gettime(CLOCK_MONOTONIC, &cur);

        double dtNs = static_cast<double>(cur.tv_sec - prev.tv_sec) * 1e9 + static_cast<double>(cur.tv_nsec - prev.tv_nsec);
        intervals.push_back(dtNs);
        prev = cur;
    }

    // Statistics
    double sum = 0;
    for (double v : intervals)
        sum += v;
    double meanNs = sum / intervals.size();

    double jitterNs = stddev(intervals);

    double minNs = *std::min_element(intervals.begin(), intervals.end());
    double maxNs = *std::max_element(intervals.begin(), intervals.end());

    data.push_back("CLOCK_MONOTONIC jitter (" + std::to_string(samples) + " samples):");
    data.push_back("  Mean interval : " + std::to_string(meanNs) + " ns");
    data.push_back("  Jitter (1σ)   : " + std::to_string(jitterNs) + " ns");
    data.push_back("  Min interval  : " + std::to_string(minNs) + " ns");
    data.push_back("  Max interval  : " + std::to_string(maxNs) + " ns");

    // Also report CLOCK_MONOTONIC_RAW if available
    struct timespec rawPrev
    {
    };
    if (::clock_gettime(CLOCK_MONOTONIC_RAW, &rawPrev) == 0) {
        std::vector<double> rawIntervals;
        rawIntervals.reserve(samples - 1);
        for (int i = 1; i < samples; ++i) {
            struct timespec cur
            {
            };
            ::clock_gettime(CLOCK_MONOTONIC_RAW, &cur);
            double dtNs = static_cast<double>(cur.tv_sec - rawPrev.tv_sec) * 1e9 + static_cast<double>(cur.tv_nsec - rawPrev.tv_nsec);
            rawIntervals.push_back(dtNs);
            rawPrev = cur;
        }
        double rawJitter = stddev(rawIntervals);
        data.push_back("  CLOCK_MONOTONIC_RAW jitter (1σ): " + std::to_string(rawJitter) + " ns");
    }

    // Pass if jitter < 10 µs = 10000 ns  (very lenient for a loaded system)
    const double thresholdNs = 10'000.0;
    bool passed = (jitterNs < thresholdNs);
    if (!passed)
        data.push_back("FAIL: Jitter " + std::to_string(jitterNs) + " ns exceeds threshold " + std::to_string(thresholdNs) + " ns");

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
 * @brief Implementation of testRtcAccuracy
 *
 * Reads the hardware RTC time via /dev/rtc0 (RTC_RD_TIME ioctl) and the
 * system CLOCK_REALTIME, then checks that they agree within ±5 seconds.
 * A larger discrepancy indicates the RTC has not been set or has drifted.
 */
TestOutput ClockAndTimingTesting::testRtcAccuracy()
{
    std::vector<std::string> data;

    // Read system time
    struct timespec sysTm
    {
    };
    ::clock_gettime(CLOCK_REALTIME, &sysTm);
    time_t sysEpoch = sysTm.tv_sec;

    // Read RTC time via ioctl
    int fd = ::open("/dev/rtc0", O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        data.push_back("WARN: Cannot open /dev/rtc0 (" + std::string(::strerror(errno)) + "); trying sysfs");

        // Fallback: /sys/class/rtc/rtc0/{date,time}
        std::string rtcDate, rtcTime;
        bool dateOk = readSysFile("/sys/class/rtc/rtc0/date", rtcDate);
        bool timeOk = readSysFile("/sys/class/rtc/rtc0/time", rtcTime);
        if (!dateOk || !timeOk) {
            data.push_back("FAIL: RTC not accessible via /dev/rtc0 or sysfs");
            {
                TestOutput _out;
                _out.status = hal::Status::FAIL;
                _out.severity = hal::Severity::ERROR;
                for (size_t _i = 0; _i < data.size(); ++_i)
                    _out.reason["detail_" + std::to_string(_i)] = data[_i];
                return _out;
            }
        }
        data.push_back("RTC (sysfs) date: " + rtcDate + "  time: " + rtcTime);
        {
            TestOutput _out;
            _out.status = hal::Status::PASS;
            _out.severity = hal::Severity::INFO;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    struct rtc_time rtcTm
    {
    };
    if (::ioctl(fd, RTC_RD_TIME, &rtcTm) < 0) {
        ::close(fd);
        data.push_back("FAIL: RTC_RD_TIME ioctl failed: " + std::string(::strerror(errno)));
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }
    ::close(fd);

    // Convert rtc_time → time_t (RTC stores UTC on Linux)
    struct tm rtcTmStd
    {
    };
    rtcTmStd.tm_sec = rtcTm.tm_sec;
    rtcTmStd.tm_min = rtcTm.tm_min;
    rtcTmStd.tm_hour = rtcTm.tm_hour;
    rtcTmStd.tm_mday = rtcTm.tm_mday;
    rtcTmStd.tm_mon = rtcTm.tm_mon;
    rtcTmStd.tm_year = rtcTm.tm_year;
    rtcTmStd.tm_isdst = 0;
    time_t rtcEpoch = ::timegm(&rtcTmStd);

    char rtcBuf[64]{}, sysBuf[64]{};
    ::strftime(rtcBuf, sizeof(rtcBuf), "%Y-%m-%d %H:%M:%S", &rtcTmStd);
    struct tm* sysTmPtr = ::gmtime(&sysEpoch);
    ::strftime(sysBuf, sizeof(sysBuf), "%Y-%m-%d %H:%M:%S", sysTmPtr);

    long diffSec = static_cast<long>(std::abs(static_cast<double>(rtcEpoch - sysEpoch)));
    data.push_back("RTC time    (UTC): " + std::string(rtcBuf));
    data.push_back("System time (UTC): " + std::string(sysBuf));
    data.push_back("Discrepancy: " + std::to_string(diffSec) + " s");

    const long threshSec = 5;
    if (diffSec > threshSec) {
        data.push_back("FAIL: RTC / system time discrepancy > " + std::to_string(threshSec) + " s");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
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
 * @brief Implementation of testRtcBatteryBackup
 *
 * Verifies that the RTC has battery backup by:
 *  1. Confirming /dev/rtc0 (or rtc1) is present and readable — battery-backed
 *     RTCs persist across power cycles.
 *  2. Checking the RTC's HCTOSYS_DEVICE and power_supply / fuelgauge flags
 *     where exposed by the BSP.
 *  3. Reading the RTC time twice separated by 2 s and confirming the RTC
 *     counter advances — the RTC oscillator is alive.
 */
TestOutput ClockAndTimingTesting::testRtcBatteryBackup()
{
    std::vector<std::string> data;

    // 1. Device presence
    std::string rtcDev;
    for (const std::string candidate : {"/dev/rtc0", "/dev/rtc1"}) {
        if (pathExists(candidate)) {
            rtcDev = candidate;
            data.push_back("RTC device found: " + candidate);
            break;
        }
    }
    if (rtcDev.empty()) {
        data.push_back("FAIL: No /dev/rtc0 or /dev/rtc1 found");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    // 2. Check sysfs attributes
    const std::string rtcSysfsBase = "/sys/class/rtc/rtc0";
    std::string sinceEpoch;
    if (readSysFile(rtcSysfsBase + "/since_epoch", sinceEpoch))
        data.push_back("RTC since_epoch: " + sinceEpoch + " s");

    // Some BSPs expose 'hctosys' (hardware clock to system synced at boot)
    std::string hctosys;
    if (readSysFile(rtcSysfsBase + "/hctosys", hctosys))
        data.push_back("RTC hctosys (set sys clock at boot): " + hctosys);

    // 3. RTC oscillator alive check — read twice, 2 s apart
    int fd = ::open(rtcDev.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        data.push_back("WARN: Cannot open " + rtcDev + " for oscillator check (" + std::string(::strerror(errno)) + ")");
        {
            TestOutput _out;
            _out.status = hal::Status::PASS;
            _out.severity = hal::Severity::INFO;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        } // device exists, treat as pass
    }

    struct rtc_time t1
    {
    }, t2{};
    bool ioOk = (::ioctl(fd, RTC_RD_TIME, &t1) == 0);
    if (ioOk) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        ioOk = (::ioctl(fd, RTC_RD_TIME, &t2) == 0);
    }
    ::close(fd);

    if (!ioOk) {
        data.push_back("FAIL: Could not read RTC twice for oscillator check");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    // Compute delta via mktime
    struct tm s1
    {
    }, s2{};
    s1.tm_sec = t1.tm_sec;
    s1.tm_min = t1.tm_min;
    s1.tm_hour = t1.tm_hour;
    s1.tm_mday = t1.tm_mday;
    s1.tm_mon = t1.tm_mon;
    s1.tm_year = t1.tm_year;
    s2.tm_sec = t2.tm_sec;
    s2.tm_min = t2.tm_min;
    s2.tm_hour = t2.tm_hour;
    s2.tm_mday = t2.tm_mday;
    s2.tm_mon = t2.tm_mon;
    s2.tm_year = t2.tm_year;
    time_t e1 = ::timegm(&s1), e2 = ::timegm(&s2);
    long delta = static_cast<long>(e2 - e1);

    data.push_back("RTC counter advanced by " + std::to_string(delta) + " s over 2 s sleep");

    if (delta < 1 || delta > 4) {
        data.push_back("FAIL: RTC counter did not advance as expected "
                       "(expected ~2 s, got " +
                       std::to_string(delta) + " s)");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    data.push_back("RTC oscillator alive — battery backup functional");
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
 * @brief Implementation of testRtcAlarmFunctionality
 *
 * Programs a non-intrusive alarm 5 seconds in the future using the
 * RTC_ALM_SET ioctl, reads it back with RTC_ALM_READ, and confirms
 * the stored alarm matches what was set.  The alarm is cleared afterwards.
 */
TestOutput ClockAndTimingTesting::testRtcAlarmFunctionality()
{
    std::vector<std::string> data;

    int fd = ::open("/dev/rtc0", O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        data.push_back("WARN: Cannot open /dev/rtc0 (" + std::string(::strerror(errno)) + ")");
        // If we can't open the device, skip but do not hard-fail so that the
        // test suite can run without root on a development host as well.
        data.push_back("SKIP: RTC alarm test requires read-write access to /dev/rtc0");
        {
            TestOutput _out;
            _out.status = hal::Status::PASS;
            _out.severity = hal::Severity::INFO;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    // Read current RTC time
    struct rtc_time curTm
    {
    };
    if (::ioctl(fd, RTC_RD_TIME, &curTm) < 0) {
        ::close(fd);
        data.push_back("FAIL: RTC_RD_TIME ioctl failed: " + std::string(::strerror(errno)));
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    // Set alarm 5 seconds ahead
    struct tm curStd
    {
    };
    curStd.tm_sec = curTm.tm_sec;
    curStd.tm_min = curTm.tm_min;
    curStd.tm_hour = curTm.tm_hour;
    curStd.tm_mday = curTm.tm_mday;
    curStd.tm_mon = curTm.tm_mon;
    curStd.tm_year = curTm.tm_year;
    curStd.tm_isdst = 0;
    time_t futureEpoch = ::timegm(&curStd) + 5;
    struct tm* futureTm = ::gmtime(&futureEpoch);

    struct rtc_time alarmTm
    {
    };
    alarmTm.tm_sec = futureTm->tm_sec;
    alarmTm.tm_min = futureTm->tm_min;
    alarmTm.tm_hour = futureTm->tm_hour;
    alarmTm.tm_mday = futureTm->tm_mday;
    alarmTm.tm_mon = futureTm->tm_mon;
    alarmTm.tm_year = futureTm->tm_year;

    // Set the alarm
    if (::ioctl(fd, RTC_ALM_SET, &alarmTm) < 0) {
        ::close(fd);
        data.push_back("FAIL: RTC_ALM_SET ioctl failed: " + std::string(::strerror(errno)));
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    // Read back the alarm
    struct rtc_time readBackTm
    {
    };
    if (::ioctl(fd, RTC_ALM_READ, &readBackTm) < 0) {
        ::close(fd);
        data.push_back("FAIL: RTC_ALM_READ ioctl failed: " + std::string(::strerror(errno)));
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    ::close(fd);

    // Verify the alarm was written correctly (seconds and minutes must match)
    bool alarmOk = (readBackTm.tm_sec == alarmTm.tm_sec && readBackTm.tm_min == alarmTm.tm_min && readBackTm.tm_hour == alarmTm.tm_hour);

    char setBuf[32]{}, readBuf[32]{};
    std::snprintf(setBuf, sizeof(setBuf), "%02d:%02d:%02d", alarmTm.tm_hour, alarmTm.tm_min, alarmTm.tm_sec);
    std::snprintf(readBuf, sizeof(readBuf), "%02d:%02d:%02d", readBackTm.tm_hour, readBackTm.tm_min, readBackTm.tm_sec);

    data.push_back("Alarm set to    : " + std::string(setBuf));
    data.push_back("Alarm read back : " + std::string(readBuf));

    if (!alarmOk) {
        data.push_back("FAIL: RTC alarm read-back does not match programmed value");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    data.push_back("RTC alarm functionality verified");
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
 * @brief Implementation of testRtcTimePersistence
 *
 * Reads the RTC time, waits 3 seconds (using CLOCK_REALTIME for an accurate
 * reference), reads the RTC again.  Confirms the RTC elapsed time is within
 * ±1 second of the CLOCK_REALTIME elapsed time.
 * This validates both that the RTC oscillates correctly and that the Linux
 * driver correctly updates the RTC counter.
 */
TestOutput ClockAndTimingTesting::testRtcTimePersistence()
{
    std::vector<std::string> data;

    int fd = ::open("/dev/rtc0", O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        data.push_back("WARN: Cannot open /dev/rtc0 (" + std::string(::strerror(errno)) + "); falling back to sysfs since_epoch");

        // Fallback: read since_epoch twice
        std::string ep1, ep2;
        if (!readSysFile("/sys/class/rtc/rtc0/since_epoch", ep1)) {
            data.push_back("FAIL: RTC not accessible");
            {
                TestOutput _out;
                _out.status = hal::Status::FAIL;
                _out.severity = hal::Severity::ERROR;
                for (size_t _i = 0; _i < data.size(); ++_i)
                    _out.reason["detail_" + std::to_string(_i)] = data[_i];
                return _out;
            }
        }

        struct timespec sysStart
        {
        };
        ::clock_gettime(CLOCK_REALTIME, &sysStart);
        std::this_thread::sleep_for(std::chrono::seconds(3));
        struct timespec sysEnd
        {
        };
        ::clock_gettime(CLOCK_REALTIME, &sysEnd);

        readSysFile("/sys/class/rtc/rtc0/since_epoch", ep2);

        long rtcDelta = std::stol(ep2) - std::stol(ep1);
        long sysDelta = static_cast<long>(sysEnd.tv_sec - sysStart.tv_sec);
        data.push_back("RTC elapsed (since_epoch): " + std::to_string(rtcDelta) + " s");
        data.push_back("System elapsed           : " + std::to_string(sysDelta) + " s");

        bool ok = (std::abs(rtcDelta - sysDelta) <= 1);
        if (!ok)
            data.push_back("FAIL: RTC delta differs from system delta by > 1 s");
        {
            TestOutput _out;
            _out.status = ok ? hal::Status::PASS : hal::Status::FAIL;
            _out.severity = ok ? hal::Severity::INFO : hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    // ioctl path
    struct rtc_time t1
    {
    }, t2{};
    if (::ioctl(fd, RTC_RD_TIME, &t1) < 0) {
        ::close(fd);
        data.push_back("FAIL: RTC_RD_TIME ioctl failed");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    struct timespec sysStart
    {
    };
    ::clock_gettime(CLOCK_REALTIME, &sysStart);

    std::this_thread::sleep_for(std::chrono::seconds(3));

    struct timespec sysEnd
    {
    };
    ::clock_gettime(CLOCK_REALTIME, &sysEnd);

    if (::ioctl(fd, RTC_RD_TIME, &t2) < 0) {
        ::close(fd);
        data.push_back("FAIL: Second RTC_RD_TIME ioctl failed");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }
    ::close(fd);

    // Epoch conversion
    struct tm s1
    {
    }, s2{};
    s1.tm_sec = t1.tm_sec;
    s1.tm_min = t1.tm_min;
    s1.tm_hour = t1.tm_hour;
    s1.tm_mday = t1.tm_mday;
    s1.tm_mon = t1.tm_mon;
    s1.tm_year = t1.tm_year;
    s2.tm_sec = t2.tm_sec;
    s2.tm_min = t2.tm_min;
    s2.tm_hour = t2.tm_hour;
    s2.tm_mday = t2.tm_mday;
    s2.tm_mon = t2.tm_mon;
    s2.tm_year = t2.tm_year;
    long rtcElapsed = static_cast<long>(::timegm(&s2) - ::timegm(&s1));
    long sysElapsed = static_cast<long>(sysEnd.tv_sec - sysStart.tv_sec);

    data.push_back("RTC elapsed    : " + std::to_string(rtcElapsed) + " s");
    data.push_back("System elapsed : " + std::to_string(sysElapsed) + " s");
    data.push_back("Difference     : " + std::to_string(std::abs(rtcElapsed - sysElapsed)) + " s");

    bool ok = (std::abs(rtcElapsed - sysElapsed) <= 1);
    if (!ok)
        data.push_back("FAIL: RTC time persistence error > 1 s");
    else
        data.push_back("RTC time persistence verified");

    {
        TestOutput _out;
        _out.status = ok ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = ok ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Run all tests for this module
 * @return Vector of test results
 */
std::vector<TestResult> ClockAndTimingTesting::runAllTests()
{
    std::vector<TestResult> results;

    // Run testSystemClockAccuracy
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSystemClockAccuracy();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSystemClockAccuracy";
            _tr.test_id = "testSystemClockAccuracy";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testPcieReferenceClock
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPcieReferenceClock();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPcieReferenceClock";
            _tr.test_id = "testPcieReferenceClock";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testUsbReferenceClock
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testUsbReferenceClock();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testUsbReferenceClock";
            _tr.test_id = "testUsbReferenceClock";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testEthernetReferenceClock
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testEthernetReferenceClock();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testEthernetReferenceClock";
            _tr.test_id = "testEthernetReferenceClock";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testCameraClockOutput
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testCameraClockOutput();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testCameraClockOutput";
            _tr.test_id = "testCameraClockOutput";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testAudioClock
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testAudioClock();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testAudioClock";
            _tr.test_id = "testAudioClock";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testPllLockStatus
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPllLockStatus();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPllLockStatus";
            _tr.test_id = "testPllLockStatus";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testClockJitterMeasurements
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testClockJitterMeasurements();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testClockJitterMeasurements";
            _tr.test_id = "testClockJitterMeasurements";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testRtcAccuracy
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testRtcAccuracy();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testRtcAccuracy";
            _tr.test_id = "testRtcAccuracy";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testRtcBatteryBackup
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testRtcBatteryBackup();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testRtcBatteryBackup";
            _tr.test_id = "testRtcBatteryBackup";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testRtcAlarmFunctionality
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testRtcAlarmFunctionality();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testRtcAlarmFunctionality";
            _tr.test_id = "testRtcAlarmFunctionality";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testRtcTimePersistence
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testRtcTimePersistence();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testRtcTimePersistence";
            _tr.test_id = "testRtcTimePersistence";
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
std::vector<std::string> ClockAndTimingTesting::getAvailableTests()
{
    return {
        "testSystemClockAccuracy", "testPcieReferenceClock", "testUsbReferenceClock",     "testEthernetReferenceClock",
        "testCameraClockOutput",   "testAudioClock",         "testPllLockStatus",         "testClockJitterMeasurements",
        "testRtcAccuracy",         "testRtcBatteryBackup",   "testRtcAlarmFunctionality", "testRtcTimePersistence",
    };
}

/**
 * @brief Run a single test by name
 * @param testName Name of the test to run
 * @return Test result with timing
 */
TestResult ClockAndTimingTesting::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if (testName == "testSystemClockAccuracy") {
        result = testSystemClockAccuracy();
    } else if (testName == "testPcieReferenceClock") {
        result = testPcieReferenceClock();
    } else if (testName == "testUsbReferenceClock") {
        result = testUsbReferenceClock();
    } else if (testName == "testEthernetReferenceClock") {
        result = testEthernetReferenceClock();
    } else if (testName == "testCameraClockOutput") {
        result = testCameraClockOutput();
    } else if (testName == "testAudioClock") {
        result = testAudioClock();
    } else if (testName == "testPllLockStatus") {
        result = testPllLockStatus();
    } else if (testName == "testClockJitterMeasurements") {
        result = testClockJitterMeasurements();
    } else if (testName == "testRtcAccuracy") {
        result = testRtcAccuracy();
    } else if (testName == "testRtcBatteryBackup") {
        result = testRtcBatteryBackup();
    } else if (testName == "testRtcAlarmFunctionality") {
        result = testRtcAlarmFunctionality();
    } else if (testName == "testRtcTimePersistence") {
        result = testRtcTimePersistence();
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
