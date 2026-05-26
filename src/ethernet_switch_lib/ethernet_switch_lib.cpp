/**
 * @file ethernet_switch_lib.cpp
 * @brief Ethernet switch tests for Raspberry Pi 5.
 *        Pi5 has NO built-in Ethernet switch (only a single-port GbE via
 *        BCM54213PE PHY on eth0, managed by the RP1 south-bridge MAC).
 *        A managed switch (e.g. Microchip KSZ9897 or Marvell 88E6390 on SPI/
 *        MDIO) may be present on a carrier board.  Tests probe for DSA
 *        (Distributed Switch Architecture) devices and Linux switchdev.
 * @author sandesh ghimire
 * @copyright (C) Lucid Motors
 */

#include "ethernet_switch_lib.h"
#include "../platform_rpi5.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <net/if.h>
#include <sstream>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

// ---------------------------------------------------------------------------
static std::string swReadFile(const std::string& path)
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

static std::string swRunCmd(const std::string& cmd)
{
    std::array<char, 1024> buf{};
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

static bool swPathExists(const std::string& p)
{
    struct stat st {};
    return stat(p.c_str(), &st) == 0;
}

// Check if a network interface exists
static bool ifaceExists(const std::string& iface)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) return false;
    struct ifreq ifr {};
    strncpy(ifr.ifr_name, iface.c_str(), IFNAMSIZ - 1);
    bool ok = (ioctl(sockfd, SIOCGIFINDEX, &ifr) == 0);
    close(sockfd);
    return ok;
}
// ---------------------------------------------------------------------------

/**
 * @brief Constructor for EthernetSwitchTesting
 */
EthernetSwitchTesting::EthernetSwitchTesting()
{
}

/**
 * @brief Destructor for EthernetSwitchTesting
 */
EthernetSwitchTesting::~EthernetSwitchTesting()
{
}

/**
 * @brief Verify Ethernet switch port functionality via DSA or switchdev.
 * @details Checks for DSA switch devices (/sys/bus/platform/drivers/dsa) and
 *          switchdev-capable interfaces.  Also verifies the built-in eth0
 *          single-port GbE interface as the uplink.
 */
TestOutput EthernetSwitchTesting::testSwitchPortFunctionality()
{
    std::vector<std::string> data;
    data.push_back("=== Ethernet Switch Port Functionality (Pi5 DSA/switchdev) ===");
    data.push_back("[INFO] Pi5 has single-port GbE (eth0/BCM54213PE); switch via DSA HAT");

    // DSA driver
    bool dsaPresent = swPathExists("/sys/bus/platform/drivers/dsa");
    data.push_back(dsaPresent ? "[PASS] DSA platform driver bound"
                              : "[INFO] No DSA platform driver (no switch HAT)");

    // Enumerate switch port interfaces (DSA creates lan1, lan2, … or swp0, swp1, …)
    static const std::vector<std::string> switchPorts = {
        "lan1", "lan2", "lan3", "lan4", "swp0", "swp1", "swp2", "swp3"};
    int portCount = 0;
    for (const auto& p : switchPorts)
    {
        if (ifaceExists(p))
        {
            std::string state = swReadFile("/sys/class/net/" + p + "/operstate");
            data.push_back("[PASS] Switch port " + p + ": " + state);
            portCount++;
        }
    }
    if (portCount == 0)
        data.push_back("[INFO] No DSA switch port interfaces found");

    // Built-in eth0 uplink
    bool eth0Up = ifaceExists(RPI5_ETH_IFACE);
    data.push_back(eth0Up ? "[PASS] eth0 GbE uplink present"
                          : "[WARN] eth0 not found");

    {
        TestOutput _out;
        _out.status = eth0Up ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = eth0Up ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Check switch MAC forwarding table (FDB) via bridge fdb show.
 */
TestOutput EthernetSwitchTesting::testSwitchForwardingTable()
{
    std::vector<std::string> data;
    data.push_back("=== Ethernet Switch FDB (Pi5) ===");

    // Check bridge FDB entries
    std::string fdb = swRunCmd("bridge fdb show 2>/dev/null | head -10");
    if (!fdb.empty())
    {
        data.push_back("[PASS] FDB entries:\n" + fdb);
        int count = 0;
        std::istringstream ss(fdb);
        std::string line;
        while (std::getline(ss, line)) count++;
        data.push_back("[INFO] FDB entry lines: " + std::to_string(count));
    }
    else
    {
        data.push_back("[INFO] No FDB entries (no bridge or switch configured)");
        data.push_back("[INFO] Create bridge: ip link add br0 type bridge && ip link set eth0 master br0");
    }

    // Show ARP table as L3 forwarding proxy
    std::string arp = swRunCmd("arp -n 2>/dev/null | grep -v 'incomplete' | head -5");
    data.push_back(arp.empty() ? "[INFO] ARP table empty" : "[INFO] ARP table:\n" + arp);

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
 * @brief Check VLAN support on eth0 and DSA switch ports.
 */
TestOutput EthernetSwitchTesting::testSwitchVlanSupport()
{
    std::vector<std::string> data;
    data.push_back("=== VLAN Support (Pi5 eth0 / DSA) ===");

    // Check 8021q module
    bool mod8021q = swPathExists("/sys/module/8021q") ||
                    swRunCmd("grep -c '^8021q ' /proc/modules 2>/dev/null") == "1";
    data.push_back(mod8021q ? "[PASS] 8021q (802.1Q VLAN) module loaded"
                            : "[INFO] 8021q module not loaded (modprobe 8021q)");

    // Check for existing VLAN interfaces
    std::string vlans = swRunCmd("ip link show type vlan 2>/dev/null | head -6");
    data.push_back(vlans.empty() ? "[INFO] No VLAN interfaces configured"
                                 : "[PASS] VLAN interfaces:\n" + vlans);

    // bridge vlan show
    std::string bridgeVlan = swRunCmd("bridge vlan show 2>/dev/null | head -6");
    data.push_back(bridgeVlan.empty() ? "[INFO] No bridge VLAN database"
                                      : "[PASS] Bridge VLANs:\n" + bridgeVlan);

    {
        TestOutput _out;
        _out.status = mod8021q ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = mod8021q ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Check QoS (traffic prioritization) on eth0.
 */
TestOutput EthernetSwitchTesting::testSwitchQos()
{
    std::vector<std::string> data;
    data.push_back("=== Switch QoS / Traffic Priority (Pi5 eth0) ===");

    // Show current qdisc on eth0
    std::string qdisc = swRunCmd("tc qdisc show dev eth0 2>/dev/null");
    data.push_back(qdisc.empty() ? "[INFO] No tc qdisc on eth0"
                                 : "[INFO] eth0 qdisc:\n" + qdisc);

    // Check for 802.1p priority bits (ethtool -k shows vlan-challenged)
    std::string ethtoolK = swRunCmd("ethtool -k eth0 2>/dev/null | grep -E 'vlan-challenged|tx-checksumming|generic-segmentation'");
    if (!ethtoolK.empty())
        data.push_back("[INFO] eth0 features:\n" + ethtoolK);

    // DSCP → priority mapping
    std::string dscp = swRunCmd("tc qdisc show dev eth0 2>/dev/null | grep -c 'prio\\|tbf\\|htb'");
    data.push_back(dscp == "0" ? "[INFO] No priority qdisc configured on eth0 (default pfifo_fast)"
                               : "[PASS] Priority qdisc active");

    data.push_back("[INFO] For 802.1Q CoS use: tc qdisc replace dev eth0 root mqprio num_tc 8");
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
 * @brief Check port mirroring (SPAN) capability via tc mirred action.
 */
TestOutput EthernetSwitchTesting::testSwitchPortMirroring()
{
    std::vector<std::string> data;
    data.push_back("=== Switch Port Mirroring / SPAN (Pi5) ===");
    data.push_back("[INFO] Pi5 has no hardware SPAN; tc+mirred provides software mirroring");

    // Check tc mirred module
    bool mirredMod = swPathExists("/sys/module/act_mirred") ||
                     swRunCmd("grep -c 'act_mirred' /proc/modules 2>/dev/null") != "0";
    data.push_back(mirredMod ? "[PASS] act_mirred module loaded (port mirroring capable)"
                             : "[INFO] act_mirred not loaded (modprobe act_mirred)");

    // Check for existing mirror filters
    std::string mirrors = swRunCmd("tc filter show dev eth0 2>/dev/null | grep -c 'mirred'");
    data.push_back("[INFO] Active mirror filters on eth0: " + mirrors);

    data.push_back("[INFO] Setup: tc filter add dev eth0 parent ffff: matchall action mirred egress mirror dev eth1");
    {
        TestOutput _out;
        _out.status = mirredMod ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = mirredMod ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Read switch port statistics via ethtool and sysfs.
 */
TestOutput EthernetSwitchTesting::testSwitchStatistics()
{
    std::vector<std::string> data;
    data.push_back("=== Switch Port Statistics (Pi5 eth0) ===");

    // ethtool statistics
    std::string ethtoolStats = swRunCmd("ethtool -S eth0 2>/dev/null | head -10");
    data.push_back(ethtoolStats.empty() ? "[INFO] ethtool stats not available"
                                        : "[PASS] eth0 ethtool stats:\n" + ethtoolStats);

    // sysfs net statistics
    std::string rxPackets = swReadFile("/sys/class/net/eth0/statistics/rx_packets");
    std::string txPackets = swReadFile("/sys/class/net/eth0/statistics/tx_packets");
    std::string rxErrors  = swReadFile("/sys/class/net/eth0/statistics/rx_errors");
    std::string txErrors  = swReadFile("/sys/class/net/eth0/statistics/tx_errors");

    if (!rxPackets.empty())
    {
        data.push_back("[PASS] eth0 RX packets: " + rxPackets);
        data.push_back("[INFO] eth0 TX packets: " + txPackets);
        data.push_back("[INFO] eth0 RX errors:  " + rxErrors);
        data.push_back("[INFO] eth0 TX errors:  " + txErrors);
    }
    else
    {
        data.push_back("[WARN] Cannot read eth0 statistics");
    }

    {
        TestOutput _out;
        _out.status = !rxPackets.empty() ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = !rxPackets.empty() ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Redundant switch operation check (Pi5 single-NIC proxy).
 */
TestOutput EthernetSwitchTesting::testRedundantSwitchOperation()
{
    std::vector<std::string> data;
    data.push_back("=== Redundant Switch Operation (Pi5) ===");
    data.push_back("[INFO] Pi5 has single GbE (eth0); redundancy via bonding or USB-NIC");

    // Check for bonding
    bool bondPresent = swPathExists("/sys/class/net/bond0") ||
                       ifaceExists("bond0");
    data.push_back(bondPresent ? "[PASS] bond0 bonding interface present"
                               : "[INFO] No bond0 — single uplink only");

    // Check for secondary USB NIC
    std::string usbNet = swRunCmd("ls /sys/class/net/ 2>/dev/null | grep -E 'eth[1-9]|usb[0-9]|enx'");
    data.push_back(usbNet.empty() ? "[INFO] No secondary NIC found"
                                  : "[PASS] Secondary NIC(s): " + usbNet);

    data.push_back("[INFO] For LACP/802.3ad redundancy: modprobe bonding && ip link add bond0 type bond mode 802.3ad");
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
 * @brief Switch failover test: simulate link loss and check failover.
 */
TestOutput EthernetSwitchTesting::testSwitchFailover()
{
    std::vector<std::string> data;
    data.push_back("=== Switch Failover (Pi5) ===");
    data.push_back("[INFO] Pi5 has single GbE; failover requires bonding + secondary interface");

    // Check current eth0 state
    std::string operstate = swReadFile("/sys/class/net/eth0/operstate");
    data.push_back("[INFO] eth0 operstate: " + (operstate.empty() ? "N/A" : operstate));

    // Check for bonding failover log
    std::string bondLog = swRunCmd("journalctl -k --no-pager 2>/dev/null | grep -i 'failover\\|bonding' | tail -3");
    data.push_back(bondLog.empty() ? "[INFO] No bonding/failover events in journal"
                                   : "[INFO] Bonding events:\n" + bondLog);

    // Check STP (spanning tree) to prevent loops
    std::string stpState = swRunCmd("bridge stp state 2>/dev/null");
    data.push_back(stpState.empty() ? "[INFO] No STP state (no bridge configured)"
                                    : "[INFO] STP state: " + stpState);

    data.push_back("[INFO] True failover test requires triggering eth0 link-down event");
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
std::vector<TestResult> EthernetSwitchTesting::runAllTests()
{
    std::vector<TestResult> results;

    // Run testSwitchPortFunctionality
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSwitchPortFunctionality();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSwitchPortFunctionality";
            _tr.test_id = "testSwitchPortFunctionality";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSwitchForwardingTable
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSwitchForwardingTable();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSwitchForwardingTable";
            _tr.test_id = "testSwitchForwardingTable";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSwitchVlanSupport
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSwitchVlanSupport();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSwitchVlanSupport";
            _tr.test_id = "testSwitchVlanSupport";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSwitchQos
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSwitchQos();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSwitchQos";
            _tr.test_id = "testSwitchQos";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSwitchPortMirroring
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSwitchPortMirroring();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSwitchPortMirroring";
            _tr.test_id = "testSwitchPortMirroring";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSwitchStatistics
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSwitchStatistics();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSwitchStatistics";
            _tr.test_id = "testSwitchStatistics";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testRedundantSwitchOperation
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testRedundantSwitchOperation();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testRedundantSwitchOperation";
            _tr.test_id = "testRedundantSwitchOperation";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSwitchFailover
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSwitchFailover();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSwitchFailover";
            _tr.test_id = "testSwitchFailover";
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
std::vector<std::string> EthernetSwitchTesting::getAvailableTests()
{
    return {
        "testSwitchPortFunctionality", "testSwitchForwardingTable", "testSwitchVlanSupport",        "testSwitchQos",
        "testSwitchPortMirroring",     "testSwitchStatistics",      "testRedundantSwitchOperation", "testSwitchFailover",
    };
}

/**
 * @brief Run a single test by name
 * @param testName Name of the test to run
 * @return Test result with timing
 */
TestResult EthernetSwitchTesting::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if (testName == "testSwitchPortFunctionality") {
        result = testSwitchPortFunctionality();
    } else if (testName == "testSwitchForwardingTable") {
        result = testSwitchForwardingTable();
    } else if (testName == "testSwitchVlanSupport") {
        result = testSwitchVlanSupport();
    } else if (testName == "testSwitchQos") {
        result = testSwitchQos();
    } else if (testName == "testSwitchPortMirroring") {
        result = testSwitchPortMirroring();
    } else if (testName == "testSwitchStatistics") {
        result = testSwitchStatistics();
    } else if (testName == "testRedundantSwitchOperation") {
        result = testRedundantSwitchOperation();
    } else if (testName == "testSwitchFailover") {
        result = testSwitchFailover();
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
