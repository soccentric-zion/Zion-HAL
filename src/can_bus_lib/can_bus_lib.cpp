/**
 * @file can_bus_lib.cpp
 * @brief Implementation file for CanBusTesting class — PS CAN 2.0B via SocketCAN
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC
 *
 * The Zynq UltraScale+ MPSoC PS provides two CAN 2.0B controllers; on KV260
 * they surface as SocketCAN interfaces (can0/can1) once the "can" driver and a
 * bitrate are configured. All tests are read-only except the loopback test,
 * which requires explicit opt-in (HAL_ALLOW_CAN_TX=1).
 */

#include "can_bus_lib.h"
#include "../platform/board_detect.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

using hal::platform::listDirectory;

namespace {

std::string cbReadLine(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::string s;
    std::getline(f, s);
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
    return s;
}

TestOutput cbOut(bool ok, const std::vector<std::string>& data)
{
    TestOutput o;
    o.status = ok ? hal::Status::PASS : hal::Status::FAIL;
    o.severity = ok ? hal::Severity::INFO : hal::Severity::ERROR;
    for (size_t i = 0; i < data.size(); ++i)
        o.reason["detail_" + std::to_string(i)] = data[i];
    return o;
}

TestOutput cbSkip(const std::vector<std::string>& data)
{
    TestOutput o;
    o.status = hal::Status::SKIPPED;
    o.severity = hal::Severity::INFO;
    for (size_t i = 0; i < data.size(); ++i)
        o.reason["detail_" + std::to_string(i)] = data[i];
    return o;
}

/// Linux ARPHRD_CAN — the interface type file for SocketCAN links reads "280".
constexpr int kArphrdCan = 280;

/// Enumerate SocketCAN interfaces present in /sys/class/net.
std::vector<std::string> enumerateCanInterfaces()
{
    std::vector<std::string> result;
    for (const auto& ifname : listDirectory("/sys/class/net")) {
        const std::string type = cbReadLine("/sys/class/net/" + ifname + "/type");
        if (type == std::to_string(kArphrdCan)) result.push_back(ifname);
    }
    std::sort(result.begin(), result.end());
    return result;
}

} // namespace

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
CanBusTesting::CanBusTesting() = default;
CanBusTesting::~CanBusTesting() = default;

// ---------------------------------------------------------------------------
// 1. Interface presence
// ---------------------------------------------------------------------------
TestOutput CanBusTesting::testCanInterfacePresence()
{
    std::vector<std::string> data;
    const auto ifaces = enumerateCanInterfaces();
    data.push_back("SocketCAN interfaces found: " + std::to_string(ifaces.size()));

    int psCanNodes = 0;
    for (const auto& d : listDirectory("/sys/bus/platform/devices")) {
        if (d.find(".can") != std::string::npos) {
            data.push_back("PS CAN controller node: " + d);
            ++psCanNodes;
        }
    }

    if (ifaces.empty() && psCanNodes == 0) {
        data.push_back("INFO: neither SocketCAN interfaces nor CAN DT nodes found.");
        data.push_back("INFO: on KV260/ZynqMP enable CONFIG_CAN + CONFIG_CAN_XILINX_ZYNQMP and add CAN DT nodes.");
        return cbSkip(data);
    }

    for (const auto& n : ifaces) data.push_back("  interface: " + n);
    if (ifaces.empty()) {
        data.push_back("INFO: CAN controllers exist in hardware (" + std::to_string(psCanNodes) +
                       ") but no SocketCAN interface is registered (driver not loaded / not configured).");
        return cbSkip(data);
    }

    data.push_back("PASS: SocketCAN interface(s) present.");
    return cbOut(true, data);
}

// ---------------------------------------------------------------------------
// 2. Link state
// ---------------------------------------------------------------------------
TestOutput CanBusTesting::testCanLinkState()
{
    const auto ifaces = enumerateCanInterfaces();
    if (ifaces.empty()) {
        return cbSkip({"SKIP: no SocketCAN interfaces present."});
    }

    std::vector<std::string> data;
    bool anyUp = false;
    for (const auto& n : ifaces) {
        std::string oper = cbReadLine("/sys/class/net/" + n + "/operstate");
        if (oper.empty()) oper = "unknown";
        data.push_back("  " + n + ": operstate=" + oper);
        if (oper == "up") anyUp = true;
    }
    if (anyUp)
        data.push_back("INFO: 'up' may be reported without a bus; verify with a peer node or loopback.");
    else
        data.push_back("INFO: no CAN interface is up — bring up with 'sudo ip link set can0 up type can bitrate 500000'.");
    return cbOut(true, data);
}

// ---------------------------------------------------------------------------
// 3. Socket open/bind per interface
// ---------------------------------------------------------------------------
TestOutput CanBusTesting::testCanSocketOpen()
{
    const auto ifaces = enumerateCanInterfaces();
    if (ifaces.empty()) {
        return cbSkip({"SKIP: no SocketCAN interfaces — open/bind cannot be exercised."});
    }

    std::vector<std::string> data;
    int total = 0, errors = 0;
    for (const auto& name : ifaces) {
        const int s = ::socket(PF_CAN, static_cast<int>(SOCK_RAW) | SOCK_CLOEXEC, CAN_RAW);
        if (s < 0) {
            data.push_back("  " + name + ": socket() failed: " + std::string(strerror(errno)));
            ++errors;
            continue;
        }
        ifreq ifr{};
        std::snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", name.c_str());
        if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
            data.push_back("  " + name + ": SIOCGIFINDEX failed: " + std::string(strerror(errno)));
            ++errors;
            close(s);
            continue;
        }
        sockaddr_can addr{};
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;
        if (::bind(s, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
            data.push_back("  " + name + ": bind failed: " + std::string(strerror(errno)));
            ++errors;
            close(s);
            continue;
        }
        data.push_back("  " + name + ": socket open + bind OK (ifindex " +
                       std::to_string(ifr.ifr_ifindex) + ")");
        close(s);
    }

    if (total == 0) return cbSkip({"SKIP: no CAN interfaces to open."});
    data.push_back(errors == 0
        ? "PASS: raw CAN socket opened and bound on all " + std::to_string(total) + " interface(s)."
        : "FAIL: CAN socket open/bind failed on one or more interfaces.");
    return cbOut(errors == 0, data);
}

// ---------------------------------------------------------------------------
// 4. Error counters
// ---------------------------------------------------------------------------
TestOutput CanBusTesting::testCanErrorCounters()
{
    std::vector<std::string> data;
    const auto ifaces = enumerateCanInterfaces();
    if (ifaces.empty()) {
        data.push_back("SKIP: no CAN interfaces — no error counters to inspect.");
        return cbSkip(data);
    }

    bool ok = true;
    for (const auto& n : ifaces) {
        const std::string base = "/sys/class/net/" + n + "/statistics/";
        long rxErrors = -1, txErrors = -1;
        try { rxErrors = std::stol(cbReadLine(base + "rx_errors")); } catch (...) {}
        try { txErrors = std::stol(cbReadLine(base + "tx_errors")); } catch (...) {}
        data.push_back("  " + n + ": rx_errors=" + std::to_string(rxErrors) +
                       " tx_errors=" + std::to_string(txErrors));
        if (rxErrors != 0 || txErrors != 0) {
            data.push_back("  WARNING: non-zero error counters on " + n +
                           " (bus-off, CRC errors, or a noisy segment).");
            ok = false;
        }
    }
    data.push_back(ok ? "PASS: CAN controller reports no errors." : "FAIL: CAN error counters are non-zero.");
    return cbOut(ok, data);
}

// ---------------------------------------------------------------------------
// 5. Loopback send/receive (guarded by HAL_ALLOW_CAN_TX)
// ---------------------------------------------------------------------------
TestOutput CanBusTesting::testCanLoopbackSendReceive()
{
    if (const char* env = std::getenv("HAL_ALLOW_CAN_TX");
        env == nullptr || std::string(env) != "1") {
        return cbSkip({"SKIP: test only runs with HAL_ALLOW_CAN_TX=1 (sends on the CAN bus otherwise)."});
    }

    const auto ifaces = enumerateCanInterfaces();
    if (ifaces.empty()) {
        return cbSkip({"SKIP: no SocketCAN interface is up."});
    }

    std::string ifname;
    for (const auto& n : ifaces) {
        const std::string operState = cbReadLine("/sys/class/net/" + n + "/operstate");
        if (operState == "up") { ifname = n; break; }
    }
    if (ifname.empty()) {
        return cbSkip({"SKIP: no CAN interface is UP — cannot transmit. Bring one up first."});
    }

    const int s = ::socket(PF_CAN, SOCK_RAW | SOCK_CLOEXEC, CAN_RAW);
    if (s < 0) return cbOut(false, {"socket() failed: " + std::string(strerror(errno))});

    // Enable local loopback so the test does not need a second node.
    int loop = 1;
    setsockopt(s, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loop, sizeof(loop));

    ifreq ifr{};
    std::snprintf(&ifr.ifr_name[0], sizeof(ifr.ifr_name), "%s", ifname.c_str());
    if (::ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        const std::string err = strerror(errno);
        ::close(s);
        return cbOut(false, {"SIOCGIFINDEX failed on " + ifname + ": " + err});
    }

    sockaddr_can addr{};
    addr.can_family = AF_CAN;
    addr.can_ifindex = static_cast<int>(ifr.ifr_ifindex);
    if (::bind(s, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        const std::string err = strerror(errno);
        ::close(s);
        return cbOut(false, {"bind failed on " + ifname + ": " + err});
    }

    can_frame tx{};
    tx.can_id = 0x123;
    tx.can_dlc = 4;
    tx.data[0] = 0xDE; tx.data[1] = 0xAD; tx.data[2] = 0xBE; tx.data[3] = 0xEF;
    if (::write(s, &tx, sizeof(tx)) != static_cast<ssize_t>(sizeof(tx))) {
        const std::string err = strerror(errno);
        ::close(s);
        return cbOut(false, {"write() failed on " + ifname + ": " + err});
    }

    can_frame rx{};
    const ssize_t got = ::read(s, &rx, sizeof(rx));
    ::close(s);

    std::vector<std::string> data;
    if (got == sizeof(rx) && rx.can_id == tx.can_id && rx.data[0] == 0xDE) {
        data.push_back("PASS: loopback frame echoed on " + ifname +
                       " (id=0x123 dlc=" + std::to_string(rx.can_dlc) + ")");
        return cbOut(true, data);
    }
    data.push_back("FAIL: loopback frame not echoed on " + ifname);
    if (got > 0) data.push_back("  note: received " + std::to_string(got) + " bytes with non-matching id/data");
    return cbOut(false, data);
}

// ---------------------------------------------------------------------------
// TestBase plumbing
// ---------------------------------------------------------------------------
std::vector<std::string> CanBusTesting::getAvailableTests()
{
    return {
        "testCanInterfacePresence",
        "testCanLinkState",
        "testCanSocketOpen",
        "testCanErrorCounters",
        "testCanLoopbackSendReceive",
    };
}

std::vector<TestResult> CanBusTesting::runAllTests()
{
    std::vector<TestResult> results;
    for (const auto& name : getAvailableTests()) results.push_back(runSingleTest(name));
    return results;
}

TestResult CanBusTesting::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if (testName == "testCanInterfacePresence") result = testCanInterfacePresence();
    else if (testName == "testCanLinkState") result = testCanLinkState();
    else if (testName == "testCanSocketOpen") result = testCanSocketOpen();
    else if (testName == "testCanErrorCounters") result = testCanErrorCounters();
    else if (testName == "testCanLoopbackSendReceive") result = testCanLoopbackSendReceive();
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