/**
 * @file rpu_lib.cpp
 * @brief Implementation file for RpuTesting class — RPU (dual Cortex-R5F) management
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC
 *
 * On the Kria KV260 the RPU (two Cortex-R5F cores at ~600 MHz) is presented by
 * the Linux kernel through the remoteproc framework:
 *   /sys/class/remoteproc/remoteproc<N>/{name,state,firmware,...}
 * The Kria Vision AI stack boots OpenAMP firmware as remoteproc instances
 * (rpu0 in split mode or lockstep, name "ff9a0000.rpu"/"r5f@ff9a0000").
 * IPC with a running firmware is exposed via the rpmsg bus (/dev/rpmsg_ctrl0).
 */

#include "rpu_lib.h"
#include "../platform/board_detect.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <endian.h>
#include <fstream>
#include <sstream>

using hal::platform::listDirectory;
using hal::platform::pathExists;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string rpReadLine(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::string s;
    std::getline(f, s);
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
    return s;
}

static TestOutput rpMakeOut(bool ok, const std::vector<std::string>& data)
{
    TestOutput o;
    o.status = ok ? hal::Status::PASS : hal::Status::FAIL;
    o.severity = ok ? hal::Severity::INFO : hal::Severity::ERROR;
    for (size_t i = 0; i < data.size(); ++i)
        o.reason["detail_" + std::to_string(i)] = data[i];
    return o;
}

static TestOutput rpSkip(const std::vector<std::string>& data)
{
    TestOutput o;
    o.status = hal::Status::SKIPPED;
    o.severity = hal::Severity::INFO;
    for (size_t i = 0; i < data.size(); ++i)
        o.reason["detail_" + std::to_string(i)] = data[i];
    return o;
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
RpuTesting::RpuTesting() = default;
RpuTesting::~RpuTesting() = default;

// ---------------------------------------------------------------------------
// listRemoteprocNames — enumerate /sys/class/remoteproc instances
// ---------------------------------------------------------------------------
std::vector<std::string> RpuTesting::listRemoteprocNames(const std::string& rproc_root)
{
    std::vector<std::string> names;
    for (const auto& d : hal::platform::listDirectory(rproc_root)) {
        if (d.rfind("remoteproc", 0) == 0) names.push_back(d);
    }
    std::sort(names.begin(), names.end());
    return names;
}

// ---------------------------------------------------------------------------
// 1. remoteproc presence
// ---------------------------------------------------------------------------
TestOutput RpuTesting::testRpuRemoteprocPresence()
{
    std::vector<std::string> data;
    data.push_back("RPU remoteproc presence (ZynqMP dual Cortex-R5F)");

    const std::string root = "/sys/class/remoteproc";
    if (!hal::platform::pathExists(root)) {
        data.push_back("INFO: " + root + " missing — remoteproc framework disabled in this kernel.");
        data.push_back("INFO: KV260 Kria images ship it; PetaLinux: CONFIG_REMOTEPROC + CONFIG_ZYNQMP_R5_REMOTEPROC.");
        return rpSkip(data);
    }

    const auto instances = listRemoteprocNames(root);
    data.push_back("remoteproc instances: " + std::to_string(instances.size()));

    bool rpuFound = false;
    for (const auto& inst : instances) {
        const std::string name = rpReadLine(root + "/" + inst + "/name");
        if (name.find("rpu") != std::string::npos || name.find("r5") != std::string::npos) {
            rpuFound = true;
            data.push_back("  RPU: " + inst + " -> " + name);
        }
    }

    if (instances.empty()) {
        data.push_back("INFO: remoteproc framework present but no instances bound.");
        data.push_back("INFO: On KV260, load an RPU-enabled DTBO: sudo xmutil loadapp kv260-<app> or check dmesg | grep r5f");
        return rpSkip(data);
    }
    if (!rpuFound) {
        data.push_back("WARNING: remoteproc instances exist but none is an RPU (r5f) — check DTB.");
        data.push_back("INFO: expected name pattern: '<hex>.rpu' or 'r5f@ff9a0000'.");
        return rpMakeOut(false, data);
    }
    data.push_back("PASS: RPU remoteproc instance found.");
    return rpMakeOut(true, data);
}

// ---------------------------------------------------------------------------
// 2. R5 core state
// ---------------------------------------------------------------------------
TestOutput RpuTesting::testRpuCoreState()
{
    std::vector<std::string> data;
    data.push_back("R5F core state probe (remoteproc state attribute)");

    const std::string root = "/sys/class/remoteproc";
    const auto instances = listRemoteprocNames(root);
    bool anyState = false, anyRunning = false;

    for (const auto& inst : instances) {
        const std::string dir = root + "/" + inst;
        const std::string name = rpReadLine(dir + "/name");
        if (name.find("rpu") == std::string::npos && name.find("r5") == std::string::npos)
            continue;
        const std::string state = rpReadLine(dir + "/state");
        const std::string fw = rpReadLine(dir + "/firmware");
        data.push_back("  " + inst + " (" + name + "): state=" + (state.empty() ? "<absent>" : state) +
                       " firmware=" + (fw.empty() ? "<absent>" : fw));
        anyState = true;
        if (state == "running" || state == "detached") anyState = true, anyRunning = true;
    }

    if (!anyState) {
        data.push_back("INFO: no R5 remoteproc instance exposed a state attribute (offline target?).");
        return rpSkip(data);
    }
    if (anyRunning)
        data.push_back("PASS: R5 firmware is executing (remoteproc state running/detached).");
    else
        data.push_back("INFO: RPU not running (expected state: offline). Boot firmware via remoteproc.");
    return rpMakeOut(true, data); // state read successful regardless of running
}

// ---------------------------------------------------------------------------
// 3. Firmware availability
// ---------------------------------------------------------------------------
TestOutput RpuTesting::testRpuFirmwareAvailability()
{
    std::vector<std::string> data;
    data.push_back("RPU firmware availability (/lib/firmware ELF images)");

    const std::string root = "/sys/class/remoteproc";
    const auto instances = listRemoteprocNames(root);

    if (instances.empty()) {
        data.push_back("INFO: no remoteproc instances — nothing to validate.");
        return rpSkip(data);
    }

    bool ok = true;
    for (const auto& inst : instances) {
        const std::string dir = root + "/" + inst;
        const std::string name = rpReadLine(dir + "/name");
        if (name.find("rpu") == std::string::npos && name.find("r5") == std::string::npos)
            continue;

        const std::string fw = rpReadLine(dir + "/firmware");
        data.push_back("  " + inst + " requests firmware: " + (fw.empty() ? "<unset>" : fw));
        if (fw.empty()) { ok = false; continue; }

        const std::string path = "/lib/firmware/" + fw;
        const bool present = hal::platform::pathExists(path);
        data.push_back(std::string("    " + path + ": ") + (present ? "PRESENT" : "MISSING"));
        if (!present) ok = false;
        if (!present) {
            data.push_back("    INFO: copy the OpenAMP ELF into /lib/firmware and echo it to firmware.");
        }
    }

    data.push_back(ok ? "PASS: all configured RPU firmware images found."
                      : "FAIL: one or more RPU firmware images missing from /lib/firmware.");
    return rpMakeOut(ok, data);
}

// ---------------------------------------------------------------------------
// 4. Operating mode (lockstep vs split)
// ---------------------------------------------------------------------------
TestOutput RpuTesting::testRpuOperatingMode()
{
    std::vector<std::string> data;
    data.push_back("RPU operating mode (lockstep vs split) from device tree / zynqmp-firmware");

    // Xilinx zynqmp_r5_remoteproc device trees carry xlnx,cluster-mode = <0 split |1 lockstep>
    // Exposed via /proc/device-tree under the RPU node.
    bool modeFound = false;
    int  clusterMode = -1;

    // Walk the device-tree looking for the cluster-mode property.
    const std::vector<std::string> candidates = {
        "/proc/device-tree/amba/rpu@ff9a0000/xlnx,cluster-mode",
        "/proc/device-tree/zynqmp-rpu/rpu@ff9a0000/xlnx,cluster-mode",
        "/sys/firmware/devicetree/base/amba/rpu@ff9a0000/xlnx,cluster-mode",
    };
    for (const auto& p : candidates) {
        if (!hal::platform::pathExists(p)) continue;
        std::ifstream f(p, std::ios::binary);
        unsigned int v = 0;
        if (f.read(reinterpret_cast<char*>(&v), sizeof(v))) {
            clusterMode = static_cast<int>(be32toh(v));
            modeFound = true;
        }
        break;
    }

    bool anyRpu = false;
    for (const auto& d : listRemoteprocNames("/sys/class/remoteproc")) {
        const std::string n = rpReadLine("/sys/class/remoteproc/" + d + "/name");
        if (n.find("rpu") != std::string::npos || n.find("r5") != std::string::npos) anyRpu = true;
    }

    if (!anyRpu && !modeFound) {
        data.push_back("INFO: no RPU node in device tree and no remoteproc instance — RPU domain not provisioned.");
        return rpSkip(data);
    }
    if (modeFound) {
        data.push_back(std::string("xlnx,cluster-mode = ") + std::to_string(clusterMode) +
                       (clusterMode == 0 ? " (split: two independent R5F cores)"
                                         : clusterMode == 1 ? " (lockstep: fail-safe redundancy)"
                                                            : " (unknown value)"));
        data.push_back("PASS: RPU operating mode identified.");
        return rpMakeOut(true, data);
    }
    data.push_back("INFO: RPU is provisioned but cluster-mode not exported by this DTB.");
    data.push_back("INFO: lockstep vs split is chosen at compile time of the firmware DTB.");
    return rpSkip(data);
}

// ---------------------------------------------------------------------------
// 5. rpmsg IPC presence
// ---------------------------------------------------------------------------
TestOutput RpuTesting::testRpmsgIpcPresence()
{
    std::vector<std::string> data;
    data.push_back("OpenAMP rpmsg IPC transport (inter-processor communication with R5)");

    const bool bus = hal::platform::pathExists("/sys/bus/rpmsg");
    const bool ctrl = hal::platform::pathExists("/dev/rpmsg_ctrl0");
    data.push_back(std::string("/sys/bus/rpmsg:           ") + (bus ? "YES" : "NO"));
    data.push_back(std::string("/dev/rpmsg_ctrl0:         ") + (ctrl ? "YES" : "NO"));

    if (!bus) {
        data.push_back("INFO: rpmsg bus absent — kernel lacks CONFIG_RPMSG / CONFIG_RPMSG_CHAR.");
        return rpSkip(data);
    }
    if (!ctrl) {
        data.push_back("INFO: rpmsg bus present but no OpenAMP firmware is running (rpmsg_ctrl0 appears only after R5 boot).");
        data.push_back("INFO: boot an OpenAMP firmware (remoteproc) to enable cross-core IPC.");
        return rpSkip(data);
    }

    // Enumerate rpmsg endpoints (virtio device nodes)
    int endpoints = 0;
    for (const auto& d : hal::platform::listDirectory("/sys/bus/rpmsg/devices")) {
        ++endpoints;
        const std::string name = rpReadLine("/sys/bus/rpmsg/devices/" + d + "/name");
        data.push_back("  endpoint: " + d + (name.empty() ? "" : " (" + name + ")"));
    }
    data.push_back("rpmsg endpoint count: " + std::to_string(endpoints));
    data.push_back("PASS: rpmsg transport available for A53<->R5 IPC.");
    return rpMakeOut(true, data);
}

// ---------------------------------------------------------------------------
// 6. Reserved memory carve-outs
// ---------------------------------------------------------------------------
TestOutput RpuTesting::testRpuReservedMemory()
{
    std::vector<std::string> data;
    data.push_back("RPU reserved-memory carve-outs (DDR regions excluded from Linux)"); 

    const std::string rmRoot = "/sys/firmware/devicetree/base/reserved-memory";
    if (!hal::platform::pathExists(rmRoot)) {
        data.push_back("INFO: no reserved-memory node under /sys/firmware/devicetree/base —");
        data.push_back("      R5 firmware likely shares all of DDR with Linux (KV260 default).");
        return rpSkip(data);
    }

    int carveOuts = 0;
    for (const auto& d : listDirectory(rmRoot)) {
        const std::string dir = rmRoot + "/" + d;
        const bool rsc = pathExists(dir + "/compatible");
        if (!rsc) continue;
        ++carveOuts;
        const std::string compat = rpReadLine(dir + "/compatible");
        const std::string regFile = dir + "/reg";
        std::string regInfo;
        std::ifstream f(regFile, std::ios::binary);
        if (f) {
            std::ostringstream ss;
            unsigned int word = 0;
            while (f.read(reinterpret_cast<char*>(&word), sizeof(word))) {
                ss << std::hex << be32toh(word) << " ";
            }
            regInfo = "reg=[be32 " + ss.str() + "]";
        }
        data.push_back("  " + d + (compat.empty() ? "" : " (" + compat + ")") + " " + regInfo);
    }

    data.push_back("carve-out count: " + std::to_string(carveOuts));
    data.push_back(carveOuts > 0
        ? "PASS: reserved-memory regions exist for asymmetric processing."
        : "INFO: reserved-memory present but plain (only Linux-side reservations).");
    return rpMakeOut(true, data);
}

// ---------------------------------------------------------------------------
// 7. Controlled RPU start/stop (guarded!)
// ---------------------------------------------------------------------------
TestOutput RpuTesting::testRpuStartStop()
{
    std::vector<std::string> data;
    data.push_back("RPU lifecycle control (remoteproc start/stop) — GUARDED");

    const char* env = std::getenv("HAL_ALLOW_RPU_CONTROL");
    if (env == nullptr || std::string(env) != "1") {
        data.push_back("SKIPPED: RPU start/stop would disturb a live safety application.");
        data.push_back("INFO: set HAL_ALLOW_RPU_CONTROL=1 to enable this destructive check.");
        return rpSkip(data);
    }

    const std::string root = "/sys/class/remoteproc";
    bool exercised = false;
    for (const auto& inst : listRemoteprocNames(root)) {
        const std::string dir = root + "/" + inst;
        const std::string name = rpReadLine(dir + "/name");
        if (name.find("rpu") == std::string::npos && name.find("r5") == std::string::npos) continue;

        const std::string before = rpReadLine(dir + "/state");
        data.push_back("  " + inst + ": state before control = " + (before.empty() ? "<absent>" : before));

        // stop (if running)
        if (before == "running") {
            std::ofstream stop(dir + "/state");
            if (stop) stop << "stop";
        }

        // start
        {
            std::ofstream start(dir + "/state");
            if (start) start << "start";
        }
        const std::string after = rpReadLine(dir + "/state");
        data.push_back("  " + inst + ": state after start command -> " + (after.empty() ? "<absent>" : after));
        exercised = true;

        data.push_back((after == "running" || after == "offline")
            ? "PASS: remoteproc lifecycle command accepted (state=" + after + ")"
            : "FAIL: unexpected RPU state '" + after + "'");
        return rpMakeOut(after == "running" || after == "offline" || after.empty(), data);
    }

    if (!exercised) {
        data.push_back("INFO: no RPU remoteproc instance to control (environment allows it).");
        return rpSkip(data);
    }
    return rpMakeOut(true, data);
}

// ---------------------------------------------------------------------------
// TestBase plumbing
// ---------------------------------------------------------------------------
std::vector<std::string> RpuTesting::getAvailableTests()
{
    return {
        "testRpuRemoteprocPresence",
        "testRpuCoreState",
        "testRpuFirmwareAvailability",
        "testRpuOperatingMode",
        "testRpmsgIpcPresence",
        "testRpuReservedMemory",
        "testRpuStartStop",
    };
}

std::vector<TestResult> RpuTesting::runAllTests()
{
    std::vector<TestResult> results;
    for (const auto& name : getAvailableTests()) results.push_back(runSingleTest(name));
    return results;
}

TestResult RpuTesting::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if (testName == "testRpuRemoteprocPresence")        result = testRpuRemoteprocPresence();
    else if (testName == "testRpuCoreState")            result = testRpuCoreState();
    else if (testName == "testRpuFirmwareAvailability") result = testRpuFirmwareAvailability();
    else if (testName == "testRpuOperatingMode")        result = testRpuOperatingMode();
    else if (testName == "testRpmsgIpcPresence")        result = testRpmsgIpcPresence();
    else if (testName == "testRpuReservedMemory")       result = testRpuReservedMemory();
    else if (testName == "testRpuStartStop")            result = testRpuStartStop();
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