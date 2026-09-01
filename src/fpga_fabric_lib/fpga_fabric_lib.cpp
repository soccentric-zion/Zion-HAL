/**
 * @file fpga_fabric_lib.cpp
 * @brief Implementation file for FpgaFabricTesting class — FPGA (PL) fabric
 *        validation for the AMD Kria KV260 (Zynq UltraScale+ MPSoC) and Zion carrier.
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC
 *
 * All tests are read-only. Bitstream loading mutates live fabric state and is
 * intentionally not automated here — use xmutil/dfx-mgr on the target and then
 * re-run this suite to validate the loaded design.
 */

#include "fpga_fabric_lib.h"
#include "../platform/board_detect.h"
#include "../ps_pl_backend/ps_pl_backend.h"
#include "../ps_pl_backend/uio_axil_backend.h"
#include "../platform_zynq.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

// ---------------------------------------------------------------------------
// Small read helpers
// ---------------------------------------------------------------------------
static std::string ffRead(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::string s;
    std::getline(f, s);
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
    return s;
}

static std::string ffTrim(const std::string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

/// Build a TestOutput from a detail list. ok=true -> PASS/INFO, else FAIL/ERROR.
static TestOutput ffOut(bool ok, const std::vector<std::string>& data)
{
    TestOutput o;
    o.status = ok ? hal::Status::PASS : hal::Status::FAIL;
    o.severity = ok ? hal::Severity::INFO : hal::Severity::ERROR;
    for (size_t i = 0; i < data.size(); ++i)
        o.reason["detail_" + std::to_string(i)] = data[i];
    return o;
}

/// Skipped outcome (hardware/tooling not present on this system).
static TestOutput ffSkip(const std::vector<std::string>& data)
{
    TestOutput o;
    o.status = hal::Status::SKIPPED;
    o.severity = hal::Severity::INFO;
    o.error_message = "";
    for (size_t i = 0; i < data.size(); ++i)
        o.reason["detail_" + std::to_string(i)] = data[i];
    return o;
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
FpgaFabricTesting::FpgaFabricTesting() = default;
FpgaFabricTesting::~FpgaFabricTesting() = default;

// ---------------------------------------------------------------------------
// 1. FPGA manager presence
// ---------------------------------------------------------------------------
TestOutput FpgaFabricTesting::testFpgaManagerPresence()
{
    std::vector<std::string> data;
    data.push_back("FPGA manager framework probe (/sys/class/fpga_manager)");

    const std::string base = "/sys/class/fpga_manager";
    const std::vector<std::string> mgrs = hal::platform::listDirectory(base);
    data.push_back("instances under " + base + ": " + std::to_string(mgrs.size()));

    if (mgrs.empty()) {
        data.push_back("INFO: No fpga_manager on this kernel. PetaLinux/Kria: CONFIG_FPGA + CONFIG_FPGA_MGR_ZYNQMP_FPGA.");
        data.push_back("INFO: PL may still have been loaded by the bootloader; state probe skipped.");
        return ffSkip(data);
    }

    bool ok = true;
    for (const auto& m : mgrs) {
        const std::string dir = base + "/" + m;
        data.push_back("  " + m + ": name=" + ffRead(dir + "/name"));
        if (!hal::platform::pathExists(dir + "/firmware") || !hal::platform::pathExists(dir + "/state"))
            ok = false;
    }
    data.push_back(ok ? "PASS: fpga_manager sysfs interface fully exposed."
                      : "FAIL: fpga missing firmware/state attribute.");
    return ffOut(ok, data);
}

// ---------------------------------------------------------------------------
// 2. FPGA manager state read
// ---------------------------------------------------------------------------
TestOutput FpgaFabricTesting::testFpgaManagerStateRead()
{
    std::vector<std::string> data;
    data.push_back("FPGA manager state read (read-only)");

    const std::string base = "/sys/class/fpga_manager/fpga0";
    if (!hal::platform::pathExists(base)) {
        data.push_back("INFO: /sys/class/fpga_manager/fpga0 not present (mainline ZynqMP name is 'fpga0').");
        return ffSkip(data);
    }

    const std::string state = ffRead(base + "/state");
    const std::string flags = ffRead(base + "/flags");
    data.push_back("state:  " + (state.empty() ? "<none>" : state));
    data.push_back("flags:  " + (flags.empty() ? "<absent>" : flags));

    if (state == "operating") {
        data.push_back("PASS: PL bitstream loaded and operating.");
        return ffOut(true, data);
    }
    if (state.empty()) {
        data.push_back("INFO: state attribute unreadable.");
        return ffSkip(data);
    }
    if (state == "offline") {
        data.push_back("WARNING: bitstream not loaded (offline). Load with: dfx-mgr_client.sh -l; xmutil loadapp <app>");
        TestOutput o = ffOut(true, data); // presence is fine; state is informational
        o.severity = hal::Severity::WARNING;
        return o;
    }
    // Partial states (writing/...) — treat as WARN
    TestOutput o = ffOut(true, data);
    o.severity = hal::Severity::WARNING;
    data.push_back("state '" + state + "' is unexpected — inspect dmesg.");
    o.reason["detail_" + std::to_string(data.size() - 1)] = data.back();
    return o;
}

// ---------------------------------------------------------------------------
// 3. PL AXI device enumeration via AMBA bus
// ---------------------------------------------------------------------------
TestOutput FpgaFabricTesting::testPlAxiDeviceEnumeration()
{
    std::vector<std::string> data;
    data.push_back("AMBA/AXI bus enumeration (PS peripherals + PL IP cores)");

    const std::string amba = "/sys/bus/amba/devices";
    const std::vector<std::string> devs = hal::platform::listDirectory(amba);

    if (devs.empty()) {
        data.push_back("INFO: /sys/bus/amba/devices empty — sysfs may need mounting or is restricted.");
        return ffSkip(data);
    }

    // PL cores mapped in the PL address space; classic examples:
    //   0xa0000000..  ZION deterministic-I/O contract window
    //   0x80000000-0xBFFFFFFF generic PL window
    int plCount = 0;
    for (const auto& d : devs) {
        const std::string modalias = ffRead(amba + "/" + d + "/modalias");
        const bool plWindow = d.rfind("a0", 0) == 0 || d.rfind("8000", 0) == 0 ||
                              d.rfind("4000", 0) == 0 || d.rfind("42", 0) == 0;
        if (plWindow) {
            ++plCount;
            if (static_cast<int>(data.size()) < 24)
                data.push_back("  PL " + d + " " + (modalias.empty() ? "?" : modalias));
        }
    }
    data.push_back("amba devices total: " + std::to_string(devs.size()) +
                   ", PL-window entries: " + std::to_string(plCount));
    data.push_back(plCount > 0
        ? "PASS: PL window devices enumerated — bitstream IP cores present."
        : "INFO: no PL-window AMBA devices — load a bitstream to expose fabric IP.");
    return ffOut(true, data);
}

// ---------------------------------------------------------------------------
// 4. PL clock control (pl0_ref..pl3_ref)
// ---------------------------------------------------------------------------
TestOutput FpgaFabricTesting::testPlClockControl()
{
    std::vector<std::string> data;
    bool ok = true;

    data.push_back("PL clock tree probe (pl0_ref..pl3_ref via zynqmp-firmware clk sysfs)");

    int found = 0;
    for (int i = 0; i < 4; ++i) {
        const std::string cname = "pl" + std::to_string(i) + "_ref";
        const std::string ratePath = "/sys/kernel/debug/clk/" + cname + "/clk_rate";
        if (hal::platform::pathExists(ratePath)) {
            std::string rate = ffRead(ratePath);
            data.push_back(cname + ": " + (rate.empty() ? "<unreadable>" : rate + " Hz"));
            if (!rate.empty() && rate != "0") ++found;
            if (rate == "0") ok = false; // enabled but parked at 0 — fabric dead clock
        }
    }

    if (found == 0) {
        data.push_back("INFO: no debugfs clock nodes visible.");
        data.push_back("INFO: mount debugfs: mount -t debugfs none /sys/kernel/debug");
        data.push_back("INFO: Kria alternative: xmutil plclk list");
        return ffSkip(data);
    }
    data.push_back(ok ? "PASS: PL clocks active with non-zero rates."
                      : "FAIL: PL clock at 0 Hz — check xmutil plclk / device tree assigned-clocks.");
    return ffOut(ok, data);
}

// ---------------------------------------------------------------------------
// 5. Kria acceleration stack (xmutil / dfx-mgr / xclbin)
// ---------------------------------------------------------------------------
TestOutput FpgaFabricTesting::testKriaAcceleratorStack()
{
    std::vector<std::string> data;
    data.push_back("Kria acceleration stack probe (KV260 vision AI tooling)");

    const bool xmutil = hal::platform::pathExists("/usr/bin/xmutil");
    const bool dfxmgr = hal::platform::pathExists("/usr/bin/dfx-mgr_client.sh") ||
                        hal::platform::pathExists("/usr/bin/dfx-mgr");
    int xclbin = 0, dtbo = 0;
    for (const auto& f : hal::platform::listDirectory("/lib/firmware/xilinx")) {
        if (f.size() > 7 && f.compare(f.size() - 7, 7, ".xclbin") == 0) ++xclbin;
        if (f.size() > 5 && f.compare(f.size() - 5, 5, ".dtbo") == 0) ++dtbo;
    }

    data.push_back(std::string("xmutil:               ") + (xmutil ? "YES" : "NO"));
    data.push_back(std::string("dfx-mgr_client.sh:    ") + (dfxmgr ? "YES" : "NO"));
    data.push_back("/lib/firmware/xilinx: " + std::to_string(xclbin) + " xclbin, " + std::to_string(dtbo) + " dtbo");

    if (!xmutil && !dfxmgr && xclbin == 0 && dtbo == 0) {
        data.push_back("INFO: No Kria acceleration tooling — expected on non-Kria or minimal images.");
        return ffSkip(data);
    }

    const bool complete = xmutil && dfxmgr && (xclbin > 0 || dtbo > 0);
    data.push_back(complete
        ? "PASS: xmutil + dfx-mgr + accelerator payloads present."
        : "WARNING: partial stack — PL app loading via dfx-mgr may not be reproducible.");
    TestOutput o = ffOut(complete, data);
    if (!complete) o.severity = hal::Severity::WARNING;
    return o;
}

// ---------------------------------------------------------------------------
// 6. PL interrupt lines
// ---------------------------------------------------------------------------
TestOutput FpgaFabricTesting::testPlInterruptLines()
{
    std::vector<std::string> data;
    data.push_back("PL interrupt-line check (/proc/interrupts)");

    std::ifstream f("/proc/interrupts");
    if (!f.is_open()) {
        data.push_back("INFO: /proc/interrupts unreadable.");
        return ffSkip(data);
    }

    int plIrq = 0;
    std::string line;
    while (std::getline(f, line)) {
        if (line.find("zynqmp_gpio") != std::string::npos ||
            line.find("axi-gpio") != std::string::npos ||
            line.find("gpio") != std::string::npos) {
            ++plIrq;
            if (plIrq <= 5) data.push_back(ffTrim(line));
        }
    }
    data.push_back("GPIO/PL interrupt entries: " + std::to_string(plIrq));
    data.push_back(plIrq > 0
        ? "PASS: interrupt controller entries visible."
        : "INFO: no PL IRQ consumers registered (expected on stock images without PL IRQ IPs).");
    return ffOut(true, data);
}

// ---------------------------------------------------------------------------
// 7. PL GPIO controller presence
// ---------------------------------------------------------------------------
TestOutput FpgaFabricTesting::testPlGpioControllerPresence()
{
    std::vector<std::string> data;
    data.push_back("GPIO controller availability (PS zynqmp-gpio + PL axi_gpio)");

    bool sawZynqmp = false, sawAxi = false;
    for (const auto& chip : hal::platform::listDirectory("/sys/bus/gpio/devices")) {
        if (chip.find("zynqmp-gpio") != std::string::npos) sawZynqmp = true;
        // AXI GPIO on PL: names carry the PL address (e.g. xxxx0000.gpio under fabric).
        if (chip.find(".gpio") != std::string::npos && chip.rfind("fe", 0) != 0) {
            // PL GPIOs are driven from fpga-region; PS one is fe...
            if (chip.find("a00") == 0 || chip.find("8001") == 0) sawAxi = true;
        }
    }
    data.push_back(std::string("PS zynqmp-gpio: ") + (sawZynqmp ? "YES" : "NO"));
    data.push_back(std::string("PL axi_gpio:    ") + (sawAxi ? "YES" : "NO (needs bitstream with AXI GPIO)"));

    data.push_back(sawZynqmp ? "PASS: PS GPIO controller present."
                             : "FAIL: no PS GPIO controller — kernel config or DTB issue.");
    return ffOut(sawZynqmp, data);
}

// ---------------------------------------------------------------------------
// 8. PS-PL register backend (contract + optional live UIO access)
// ---------------------------------------------------------------------------
TestOutput FpgaFabricTesting::testPsPlRegisterBackend()
{
    std::vector<std::string> data;
    data.push_back("PS-PL AXI register backend (contract mock + optional /dev/uioN)");

    // 1) Contract backend (pure, always available)
    PsPlBackend backend;
    std::uint32_t v = 0;
    bool ok = backend.readReg(ZYNQ_PL_REG_ID, v) && v == ZYNQ_PL_AXI_REG_MAGIC;
    data.push_back(ok ? "mock backend REG_ID matches 'ZION' magic"
                      : "FAIL: mock backend REG_ID corrupted");

    // 2) Live access: env HAL_PL_UIO=/dev/uioN
    if (const char* uio = std::getenv("HAL_PL_UIO"); uio && *uio) {
        auto live = UioAxilBackend::open(uio);
        if (live) {
            std::uint32_t id = 0;
            const std::string idHex = live->readReg(ZYNQ_PL_REG_ID, id)
                ? (std::ostringstream{} << std::hex << "0x" << id).str()
                : std::string("<read failed>");
            data.push_back("live " + std::string(uio) + " map=" + std::to_string(live->mapBytes()) +
                           "B REG_ID=" + idHex);
            if (!live->mapBytes()) ok = false;
            data.push_back("PASS: live UIO AXI window mapped.");
        } else {
            data.push_back("live UIO open failed: HAL_PL_UIO=" + std::string(uio));
            ok = false;
        }
    } else {
        data.push_back("INFO: live UIO AXI access disabled (set HAL_PL_UIO=/dev/uio0 to enable).");
    }

    data.push_back(ok ? "PASS: register backend contract validated." : "FAIL: register backend inconsistent.");
    return ffOut(ok, data);
}
// ---------------------------------------------------------------------------
// TestBase plumbing
// ---------------------------------------------------------------------------
std::vector<std::string> FpgaFabricTesting::getAvailableTests()
{
    return {
        "testFpgaManagerPresence",
        "testFpgaManagerStateRead",
        "testPlAxiDeviceEnumeration",
        "testPlClockControl",
        "testKriaAcceleratorStack",
        "testPlInterruptLines",
        "testPlGpioControllerPresence",
        "testPsPlRegisterBackend",
    };
}

std::vector<TestResult> FpgaFabricTesting::runAllTests()
{
    std::vector<TestResult> results;
    for (const auto& name : getAvailableTests()) results.push_back(runSingleTest(name));
    return results;
}

TestResult FpgaFabricTesting::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if (testName == "testFpgaManagerPresence") result = testFpgaManagerPresence();
    else if (testName == "testFpgaManagerStateRead") result = testFpgaManagerStateRead();
    else if (testName == "testPlAxiDeviceEnumeration") result = testPlAxiDeviceEnumeration();
    else if (testName == "testPlClockControl") result = testPlClockControl();
    else if (testName == "testKriaAcceleratorStack") result = testKriaAcceleratorStack();
    else if (testName == "testPlInterruptLines") result = testPlInterruptLines();
    else if (testName == "testPlGpioControllerPresence") result = testPlGpioControllerPresence();
    else if (testName == "testPsPlRegisterBackend") result = testPsPlRegisterBackend();
    else {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        TestResult tr;
        tr.name = testName + " (UNKNOWN)";
        tr.test_id = testName;
        tr.output.status = hal::Status::ERROR;
        tr.output.severity = hal::Severity::ERROR;
        tr.output.error_message = "Unknown test name: " + testName;
        tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
        return tr;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    TestResult tr;
    tr.name = testName;
    tr.test_id = testName;
    tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
    tr.output = result;
    return tr;
}
