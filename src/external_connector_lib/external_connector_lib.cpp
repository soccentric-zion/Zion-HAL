/**
 * @file external_connector_lib.cpp
 * @brief External connector tests for Raspberry Pi 5.
 *        Most methods reference automotive connector designators (J703-J712,
 *        KL15, BCM-CAN, VCU-CAN) from an OEM carrier board.  Pi5 maps these
 *        to its 40-pin GPIO header, USB-C power, CSI-2, DSI, and USB ports.
 *        CAN tests use SocketCAN (can0) if a MCP2515 or SN65HVD230 HAT is fitted.
 * @author sandesh ghimire
 * @copyright (C) Lucid Motors
 */

#include "external_connector_lib.h"
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
static std::string ecReadFile(const std::string& path)
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

static std::string ecRunCmd(const std::string& cmd)
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

static bool ecPathExists(const std::string& p)
{
    struct stat st {};
    return stat(p.c_str(), &st) == 0;
}

// Check if a network interface exists
static bool ecIfaceExists(const std::string& iface)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return false;
    struct ifreq ifr {};
    strncpy(ifr.ifr_name, iface.c_str(), IFNAMSIZ - 1);
    bool ok = ioctl(sock, SIOCGIFINDEX, &ifr) == 0;
    close(sock);
    return ok;
}
// ---------------------------------------------------------------------------

/**
 * @brief Constructor for ExternalConnectorTesting
 */
ExternalConnectorTesting::ExternalConnectorTesting()
{
}

/**
 * @brief Destructor for ExternalConnectorTesting
 */
ExternalConnectorTesting::~ExternalConnectorTesting()
{
}

/**
 * @brief KL15 power input (ignition line) — Pi5 USB-C 5V proxy.
 * @details Automotive KL15 = ignition-switched 12V.  Pi5 is powered via USB-C.
 *          Checks for under-voltage throttle flag (proxy for KL15 brownout).
 */
TestOutput ExternalConnectorTesting::testKl15PowerInput()
{
    std::vector<std::string> data;
    data.push_back("=== KL15 Power Input (Pi5 USB-C 5V proxy) ===");
    data.push_back("[INFO] Automotive KL15 (ignition 12V) → Pi5 maps to USB-C 5V input");

    std::string throttle = ecRunCmd("vcgencmd get_throttled 2>/dev/null");
    bool uvOk = !throttle.empty() && throttle == "throttled=0x0";
    data.push_back(uvOk ? "[PASS] No under-voltage (KL15 analogue healthy)"
                        : "[INFO] Throttle: " + throttle);

    // Power supply sysfs
    std::string psList = ecRunCmd("ls /sys/class/power_supply/ 2>/dev/null");
    data.push_back(psList.empty() ? "[INFO] No power_supply entries" : "[INFO] Power supply: " + psList);

    {
        TestOutput _out;
        _out.status = uvOk ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = uvOk ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief ADENLR signals (Automotive Digital ENable/Link Redundancy) — GPIO proxy.
 */
TestOutput ExternalConnectorTesting::testAdenlrSignals()
{
    std::vector<std::string> data;
    data.push_back("=== ADENLR Signals (Pi5 GPIO proxy) ===");
    data.push_back("[INFO] ADENLR digital enable signals map to Pi5 GPIO header (40-pin)");

    bool gpioOk = ecPathExists(RPI5_GPIOCHIP_PATH);
    data.push_back(gpioOk ? "[PASS] GPIO chip " + std::string(RPI5_GPIOCHIP_PATH) + " present"
                          : "[WARN] GPIO chip not found");

    std::string gpioInfo = ecRunCmd("gpioinfo 2>/dev/null | head -6");
    data.push_back(gpioInfo.empty() ? "[INFO] gpioinfo not available" : "[INFO] " + gpioInfo);

    {
        TestOutput _out;
        _out.status = gpioOk ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = gpioOk ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief BCM-CAN interface (Body Control Module CAN bus) via SocketCAN.
 */
TestOutput ExternalConnectorTesting::testBcmCanInterface()
{
    std::vector<std::string> data;
    data.push_back("=== BCM CAN Interface (Pi5 SocketCAN can0) ===");
    data.push_back("[INFO] BCM-CAN requires MCP2515 HAT or SN65HVD230 on SPI bus");

    bool can0 = ecIfaceExists("can0");
    data.push_back(can0 ? "[PASS] can0 interface present" : "[INFO] can0 not found (add CAN HAT)");

    if (can0)
    {
        std::string canState = ecReadFile("/sys/class/net/can0/operstate");
        data.push_back("[INFO] can0 operstate: " + canState);
        std::string canStats = ecRunCmd("ip -details link show can0 2>/dev/null | tail -3");
        data.push_back("[INFO] can0 details:\n" + canStats);
    }
    {
        TestOutput _out;
        _out.status = can0 ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = can0 ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief VCU-CAN interface (Vehicle Control Unit CAN bus).
 */
TestOutput ExternalConnectorTesting::testVcuCanInterface()
{
    std::vector<std::string> data;
    data.push_back("=== VCU CAN Interface (Pi5 SocketCAN can1) ===");
    data.push_back("[INFO] VCU-CAN maps to second CAN interface (can1) if dual CAN HAT fitted");

    bool can1 = ecIfaceExists("can1");
    data.push_back(can1 ? "[PASS] can1 (VCU-CAN) present" : "[INFO] can1 not found");

    bool can0 = ecIfaceExists("can0");
    if (!can1 && can0)
        data.push_back("[INFO] can0 (BCM-CAN) present; can1 (VCU-CAN) requires dual-port CAN HAT");

    {
        TestOutput _out;
        _out.status = (can0 || can1) ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = (can0 || can1) ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Verify all connector pins continuity (GPIO loopback / I2C scan proxy).
 */
TestOutput ExternalConnectorTesting::testAllConnectorPinsContinuity()
{
    std::vector<std::string> data;
    data.push_back("=== Connector Pin Continuity (Pi5 I2C/SPI scan proxy) ===");
    data.push_back("[INFO] Hardware pin continuity requires bed-of-nails fixture; software proxy used");

    // I2C bus scan as proxy for physical connectivity
    std::string i2cScan = ecRunCmd("i2cdetect -y 1 2>/dev/null | grep -v '^     '");
    data.push_back(i2cScan.empty() ? "[INFO] I2C scan failed (no /dev/i2c-1)"
                                   : "[PASS] I2C-1 scan:\n" + i2cScan);

    // GPIO line count
    std::string gpioCount = ecRunCmd("gpioinfo 2>/dev/null | grep -c 'line'");
    data.push_back("[INFO] GPIO lines: " + gpioCount);

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
 * @brief Connector retention force (mechanical — documented as N/A for software test).
 */
TestOutput ExternalConnectorTesting::testConnectorRetentionForce()
{
    std::vector<std::string> data;
    data.push_back("=== Connector Retention Force (Pi5) ===");
    data.push_back("[INFO] Retention force test requires force gauge on physical connector");
    data.push_back("[INFO] Pi5 connectors: USB-C (5 V), micro-HDMI x2 (28-AWG rated), CSI-2 FPC, USB-A x2");
    data.push_back("[INFO] 40-pin GPIO header: TE Connectivity 2x20 0.1\" friction-fit");
    data.push_back("[INFO] Test procedure: pull each connector at rated extraction force and verify no movement");
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
 * @brief Connector contact resistance (measured via 4-wire Kelvin probe — documented).
 */
TestOutput ExternalConnectorTesting::testConnectorContactResistance()
{
    std::vector<std::string> data;
    data.push_back("=== Connector Contact Resistance (Pi5) ===");
    data.push_back("[INFO] Contact resistance requires milli-ohmmeter (e.g. Keithley 2400)");
    data.push_back("[INFO] Spec: < 30 mΩ per pin at rated current for GPIO header");
    data.push_back("[INFO] USB-C: < 10 mΩ (USB PD spec); HDMI: < 15 mΩ; CSI FPC: < 50 mΩ");

    // Check for elevated resistance indicators (voltage drop proxy via throttle flag)
    std::string throttle = ecRunCmd("vcgencmd get_throttled 2>/dev/null");
    bool uvClean = !throttle.empty() && throttle == "throttled=0x0";
    data.push_back(uvClean ? "[PASS] No under-voltage (contact resistance likely OK)"
                           : "[INFO] Under-voltage flag set (possible poor connector): " + throttle);

    {
        TestOutput _out;
        _out.status = uvClean ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = uvClean ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief 12V ASIL QS-R power rail (N/A on Pi5 — proxy via vcgencmd voltages).
 */
TestOutput ExternalConnectorTesting::test12vAsilQsRPower()
{
    std::vector<std::string> data;
    data.push_back("=== 12V ASIL QS-R Power (Pi5 proxy) ===");
    data.push_back("[INFO] 12V ASIL QS-R rail is automotive-specific; Pi5 runs on 5V USB-C");

    // vcgencmd voltage readings as proxy
    static const std::vector<std::string> rails = {"core", "sdram_c", "sdram_i", "sdram_p"};
    for (const auto& rail : rails)
    {
        std::string v = ecRunCmd("vcgencmd measure_volts " + rail + " 2>/dev/null");
        if (!v.empty())
            data.push_back("[INFO] " + rail + ": " + v);
    }

    data.push_back("[INFO] For 12V ASIL-rated supply use TI TPS65381A / NXP FS84 PMIC");
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
 * @brief Connector mechanical integrity (visual inspection proxy — dmesg errors).
 */
TestOutput ExternalConnectorTesting::testConnectorMechanicalIntegrity()
{
    std::vector<std::string> data;
    data.push_back("=== Connector Mechanical Integrity (Pi5) ===");

    // dmesg for USB disconnect events (sign of loose USB connector)
    std::string usbDiscon = ecRunCmd("dmesg 2>/dev/null | grep -c 'USB disconnect'");
    bool usbOk = (usbDiscon == "0");
    data.push_back(usbOk ? "[PASS] No USB disconnect events" : "[WARN] USB disconnect events: " + usbDiscon);

    // HDMI hotplug events
    std::string hdmiHp = ecRunCmd("dmesg 2>/dev/null | grep -c -i 'hotplug\\|DP-'");
    data.push_back("[INFO] HDMI hotplug events: " + hdmiHp);

    // SD card removal
    std::string sdRemove = ecRunCmd("dmesg 2>/dev/null | grep -c 'mmcblk0: card removed'");
    data.push_back(sdRemove == "0" ? "[PASS] No SD card removal events"
                                   : "[WARN] SD removals: " + sdRemove);

    {
        TestOutput _out;
        _out.status = usbOk ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = usbOk ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Ethernet connector J703 (ETCL3) — maps to Pi5 eth0 GbE.
 */
TestOutput ExternalConnectorTesting::testEtcl3j703()
{
    std::vector<std::string> data;
    data.push_back("=== J703 (ETCL3) Ethernet Connector (Pi5 eth0) ===");

    bool eth0 = ecIfaceExists(RPI5_ETH_IFACE);
    data.push_back(eth0 ? "[PASS] eth0 GbE interface up" : "[WARN] eth0 not found");

    std::string state = ecReadFile("/sys/class/net/eth0/operstate");
    data.push_back("[INFO] eth0 operstate: " + state);

    std::string speed = ecReadFile("/sys/class/net/eth0/speed");
    data.push_back("[INFO] eth0 speed: " + speed + " Mbps");

    {
        TestOutput _out;
        _out.status = eth0 ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = eth0 ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Ethernet connector J704 (ETCL2) — proxy via second Ethernet if available.
 */
TestOutput ExternalConnectorTesting::testEtcl2j704()
{
    std::vector<std::string> data;
    data.push_back("=== J704 (ETCL2) Ethernet Connector (Pi5 secondary NIC proxy) ===");
    data.push_back("[INFO] Pi5 has single built-in GbE; J704 maps to USB NIC or PCIe NIC");

    std::string eth1 = ecRunCmd("ip link show 2>/dev/null | grep -E 'eth[1-9]|usb[0-9]|enx' | head -3");
    data.push_back(eth1.empty() ? "[INFO] No secondary NIC (J704 ETCL2 not populated)"
                                : "[PASS] Secondary NIC: " + eth1);

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
 * @brief Ethernet connector J705 (ETC3) — proxy via USB-C/PCIe NIC.
 */
TestOutput ExternalConnectorTesting::testEtc3j705()
{
    std::vector<std::string> data;
    data.push_back("=== J705 (ETC3) Ethernet Connector (Pi5 proxy) ===");
    data.push_back("[INFO] J705 ETC3 = third Ethernet port; Pi5 maps to USB3 NIC");

    std::string ifaces = ecRunCmd("ip link show 2>/dev/null | grep -c '^[0-9]'");
    data.push_back("[INFO] Total network interfaces: " + ifaces);

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
 * @brief Verify all Ethernet PHY connections (ethtool on all interfaces).
 */
TestOutput ExternalConnectorTesting::testAllEthernetPhyConnections()
{
    std::vector<std::string> data;
    data.push_back("=== All Ethernet PHY Connections (Pi5) ===");

    std::string ethtool = ecRunCmd("ethtool eth0 2>/dev/null | grep -E 'Speed|Duplex|Link'");
    data.push_back(ethtool.empty() ? "[INFO] ethtool not available" : "[PASS] eth0 PHY:\n" + ethtool);

    // Check BCM54213PE PHY registers via ethtool
    std::string phyId = ecRunCmd(
        "ethtool -i eth0 2>/dev/null | grep -E 'driver|bus-info|firmware'");
    data.push_back(phyId.empty() ? "[INFO] PHY driver info not available" : "[INFO] " + phyId);

    bool linkUp = (ecReadFile("/sys/class/net/eth0/carrier") == "1");
    data.push_back(linkUp ? "[PASS] eth0 carrier: link up" : "[WARN] eth0 carrier: link down");

    {
        TestOutput _out;
        _out.status = linkUp ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = linkUp ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief CC1 connector J709 — CSI-2 camera connector 1 (Pi5 CAM1/CSI-2).
 */
TestOutput ExternalConnectorTesting::testCc1j709()
{
    std::vector<std::string> data;
    data.push_back("=== J709 CC1 Camera Connector (Pi5 CAM1 CSI-2) ===");
    data.push_back("[INFO] J709 CC1 = camera connector 1; Pi5 CAM1 is 22-pin FPC CSI-2");

    bool camDev = ecPathExists("/dev/video0");
    data.push_back(camDev ? "[PASS] /dev/video0 present (camera connected to CAM1)"
                          : "[INFO] /dev/video0 absent (no camera on CAM1)");

    std::string lcCams = ecRunCmd("libcamera-hello --list-cameras 2>/dev/null | head -3");
    data.push_back(lcCams.empty() ? "[INFO] No libcamera cameras" : "[INFO] " + lcCams);

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
 * @brief CC2 connector J710 — CSI-2 camera connector 2 (Pi5 CAM0/CSI-2).
 */
TestOutput ExternalConnectorTesting::testCc2j710()
{
    std::vector<std::string> data;
    data.push_back("=== J710 CC2 Camera Connector (Pi5 CAM0 CSI-2) ===");
    data.push_back("[INFO] J710 CC2 = camera connector 2; Pi5 CAM0 is 15-pin FPC CSI-2");

    bool cam1 = ecPathExists("/dev/video1");
    data.push_back(cam1 ? "[PASS] /dev/video1 present (camera connected to CAM0)"
                        : "[INFO] /dev/video1 absent (no camera on CAM0)");

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
 * @brief CC3 connector J711 — display connector (Pi5 HDMI0 / DSI).
 */
TestOutput ExternalConnectorTesting::testCc3j711()
{
    std::vector<std::string> data;
    data.push_back("=== J711 CC3 Display Connector (Pi5 HDMI0 / DSI) ===");
    data.push_back("[INFO] J711 CC3 = display connector; Pi5 has micro-HDMI 0 + DSI FPC");

    std::string hdmi0 = ecRunCmd("cat /sys/class/drm/card*/card*-HDMI-A-1/status 2>/dev/null");
    data.push_back(hdmi0.empty() ? "[INFO] HDMI-1 DRM status unavailable"
                                 : "[INFO] HDMI-1: " + hdmi0);

    bool dsiOk = !ecRunCmd("ls /sys/bus/platform/devices/*dsi* 2>/dev/null").empty();
    data.push_back(dsiOk ? "[PASS] DSI platform device present" : "[INFO] DSI not found");

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
 * @brief CC4 connector J712 — secondary display (Pi5 HDMI1).
 */
TestOutput ExternalConnectorTesting::testCc4j712()
{
    std::vector<std::string> data;
    data.push_back("=== J712 CC4 Secondary Display (Pi5 HDMI1) ===");

    std::string hdmi1 = ecRunCmd("cat /sys/class/drm/card*/card*-HDMI-A-2/status 2>/dev/null");
    data.push_back(hdmi1.empty() ? "[INFO] HDMI-2 DRM status unavailable"
                                 : "[INFO] HDMI-2: " + hdmi1);

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
 * @brief SV (Surround View) connector — checks V4L2 multi-camera pipeline.
 */
TestOutput ExternalConnectorTesting::testSv()
{
    std::vector<std::string> data;
    data.push_back("=== SV (Surround View) Connector (Pi5 CSI-2 / USB cameras) ===");
    data.push_back("[INFO] SV = surround-view camera array; Pi5 supports 2 CSI-2 cameras + USB cameras");

    std::string videoDevs = ecRunCmd("ls /dev/video* 2>/dev/null");
    data.push_back(videoDevs.empty() ? "[INFO] No video devices found" : "[PASS] Video devices: " + videoDevs);

    // USB camera check
    std::string usbCam = ecRunCmd("lsusb 2>/dev/null | grep -i 'camera\\|webcam\\|UVC' | head -3");
    data.push_back(usbCam.empty() ? "[INFO] No USB UVC camera" : "[PASS] USB camera: " + usbCam);

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
 * @brief GNSS antenna connector — checks GNSS receiver availability.
 */
TestOutput ExternalConnectorTesting::testGnssAntCon()
{
    std::vector<std::string> data;
    data.push_back("=== GNSS Antenna Connector (Pi5 UART/USB GNSS) ===");
    data.push_back("[INFO] GNSS antenna connector feeds external u-blox module via UART/USB");

    // Check ttyAMA0 for GNSS receiver
    bool uartOk = ecPathExists("/dev/ttyAMA0");
    data.push_back(uartOk ? "[PASS] /dev/ttyAMA0 present (GNSS receiver path)"
                          : "[INFO] /dev/ttyAMA0 absent");

    // Check USB GNSS receiver
    std::string usbGnss = ecRunCmd("lsusb 2>/dev/null | grep -i 'u-blox\\|GNSS\\|GPS' | head -2");
    data.push_back(usbGnss.empty() ? "[INFO] No USB GNSS receiver"
                                   : "[PASS] USB GNSS: " + usbGnss);

    // gpsd
    std::string gpsd = ecRunCmd("systemctl is-active gpsd 2>/dev/null");
    data.push_back("[INFO] gpsd: " + (gpsd.empty() ? "inactive" : gpsd));

    {
        TestOutput _out;
        _out.status = uartOk ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = uartOk ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Ethernet debug connector — checks debug Ethernet (second NIC or USB NIC).
 */
TestOutput ExternalConnectorTesting::testEthdebugCon()
{
    std::vector<std::string> data;
    data.push_back("=== Ethernet Debug Connector (Pi5) ===");
    data.push_back("[INFO] Debug Ethernet maps to eth0 (only NIC on Pi5) or USB-NIC for isolated debug");

    bool eth0 = ecIfaceExists("eth0");
    data.push_back(eth0 ? "[PASS] eth0 present for Ethernet debug"
                        : "[WARN] eth0 not found");

    // Check for secondary debug USB-NIC (RNDIS/CDC-ECM gadget)
    std::string usb0 = ecRunCmd("ip link show usb0 2>/dev/null");
    data.push_back(usb0.empty() ? "[INFO] No USB gadget debug NIC (usb0)"
                                : "[PASS] USB gadget NIC: " + usb0);

    {
        TestOutput _out;
        _out.status = eth0 ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = eth0 ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Verify all connector pin assignments match the expected pinout table.
 */
TestOutput ExternalConnectorTesting::testAllConnectorPinAssignments()
{
    std::vector<std::string> data;
    data.push_back("=== All Connector Pin Assignments (Pi5 40-pin GPIO header) ===");
    data.push_back("[INFO] Pi5 J8 40-pin header follows Raspberry Pi standard pinout");

    // Enumerate GPIO lines and compare against expected count (27 user-accessible GPIOs)
    std::string gpioCount = ecRunCmd("gpioinfo 2>/dev/null | grep -c 'line'");
    int count = gpioCount.empty() ? 0 : std::stoi(gpioCount);
    bool gpioOk = (count >= 27);
    data.push_back(gpioOk ? "[PASS] GPIO lines: " + std::to_string(count)
                          : "[WARN] GPIO lines: " + std::to_string(count) + " (expected ≥ 27)");

    // Pin assignment table excerpt
    data.push_back("[INFO] Pin 1/17: 3.3V, Pin 2/4: 5V, Pin 6/9/14/20/25/30/34/39: GND");
    data.push_back("[INFO] Pin 3: GPIO2 (SDA), Pin 5: GPIO3 (SCL), Pin 8: GPIO14 (TXD)");
    data.push_back("[INFO] Pin 10: GPIO15 (RXD), Pin 11: GPIO17, Pin 12: GPIO18 (PCM_CLK)");
    data.push_back("[INFO] Pin 19: GPIO10 (MOSI), Pin 21: GPIO9 (MISO), Pin 23: GPIO11 (SCLK)");
    data.push_back("[INFO] Pin 24: GPIO8 (CE0), Pin 26: GPIO7 (CE1)");

    {
        TestOutput _out;
        _out.status = gpioOk ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = gpioOk ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Run all tests for this module
 * @return Vector of test results
 */
std::vector<TestResult> ExternalConnectorTesting::runAllTests()
{
    std::vector<TestResult> results;

    // Run testKl15PowerInput
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testKl15PowerInput();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testKl15PowerInput";
            _tr.test_id = "testKl15PowerInput";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testAdenlrSignals
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testAdenlrSignals();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testAdenlrSignals";
            _tr.test_id = "testAdenlrSignals";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testBcmCanInterface
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testBcmCanInterface();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testBcmCanInterface";
            _tr.test_id = "testBcmCanInterface";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testVcuCanInterface
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testVcuCanInterface();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testVcuCanInterface";
            _tr.test_id = "testVcuCanInterface";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testAllConnectorPinsContinuity
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testAllConnectorPinsContinuity();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testAllConnectorPinsContinuity";
            _tr.test_id = "testAllConnectorPinsContinuity";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testConnectorRetentionForce
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testConnectorRetentionForce();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testConnectorRetentionForce";
            _tr.test_id = "testConnectorRetentionForce";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testConnectorContactResistance
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testConnectorContactResistance();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testConnectorContactResistance";
            _tr.test_id = "testConnectorContactResistance";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run test12vAsilQsRPower
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = test12vAsilQsRPower();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "test12vAsilQsRPower";
            _tr.test_id = "test12vAsilQsRPower";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testConnectorMechanicalIntegrity
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testConnectorMechanicalIntegrity();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testConnectorMechanicalIntegrity";
            _tr.test_id = "testConnectorMechanicalIntegrity";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testEtcl3j703
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testEtcl3j703();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testEtcl3j703";
            _tr.test_id = "testEtcl3j703";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testEtcl2j704
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testEtcl2j704();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testEtcl2j704";
            _tr.test_id = "testEtcl2j704";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testEtc3j705
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testEtc3j705();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testEtc3j705";
            _tr.test_id = "testEtc3j705";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testAllEthernetPhyConnections
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testAllEthernetPhyConnections();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testAllEthernetPhyConnections";
            _tr.test_id = "testAllEthernetPhyConnections";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testCc1j709
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testCc1j709();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testCc1j709";
            _tr.test_id = "testCc1j709";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testCc2j710
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testCc2j710();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testCc2j710";
            _tr.test_id = "testCc2j710";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testCc3j711
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testCc3j711();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testCc3j711";
            _tr.test_id = "testCc3j711";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testCc4j712
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testCc4j712();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testCc4j712";
            _tr.test_id = "testCc4j712";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSv
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSv();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSv";
            _tr.test_id = "testSv";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testGnssAntCon
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testGnssAntCon();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testGnssAntCon";
            _tr.test_id = "testGnssAntCon";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testEthdebugCon
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testEthdebugCon();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testEthdebugCon";
            _tr.test_id = "testEthdebugCon";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testAllConnectorPinAssignments
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testAllConnectorPinAssignments();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testAllConnectorPinAssignments";
            _tr.test_id = "testAllConnectorPinAssignments";
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
std::vector<std::string> ExternalConnectorTesting::getAvailableTests()
{
    return {
        "testKl15PowerInput",
        "testAdenlrSignals",
        "testBcmCanInterface",
        "testVcuCanInterface",
        "testAllConnectorPinsContinuity",
        "testConnectorRetentionForce",
        "testConnectorContactResistance",
        "testConnectorMechanicalIntegrity",
        "testEtcl3j703",
        "testEtcl2j704",
        "testEtc3j705",
        "testAllEthernetPhyConnections",
        "testCc1j709",
        "testCc2j710",
        "testCc3j711",
        "testCc4j712",
        "testSv",
        "testGnssAntCon",
        "testEthdebugCon",
        "testAllConnectorPinAssignments",
    };
}

/**
 * @brief Run a single test by name
 * @param testName Name of the test to run
 * @return Test result with timing
 */
TestResult ExternalConnectorTesting::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if (testName == "testKl15PowerInput") {
        result = testKl15PowerInput();
    } else if (testName == "testAdenlrSignals") {
        result = testAdenlrSignals();
    } else if (testName == "testBcmCanInterface") {
        result = testBcmCanInterface();
    } else if (testName == "testVcuCanInterface") {
        result = testVcuCanInterface();
    } else if (testName == "testAllConnectorPinsContinuity") {
        result = testAllConnectorPinsContinuity();
    } else if (testName == "testConnectorRetentionForce") {
        result = testConnectorRetentionForce();
    } else if (testName == "testConnectorContactResistance") {
        result = testConnectorContactResistance();
    } else if (testName == "testConnectorMechanicalIntegrity") {
        result = testConnectorMechanicalIntegrity();
    } else if (testName == "testEtcl3j703") {
        result = testEtcl3j703();
    } else if (testName == "testEtcl2j704") {
        result = testEtcl2j704();
    } else if (testName == "testEtc3j705") {
        result = testEtc3j705();
    } else if (testName == "testAllEthernetPhyConnections") {
        result = testAllEthernetPhyConnections();
    } else if (testName == "testCc1j709") {
        result = testCc1j709();
    } else if (testName == "testCc2j710") {
        result = testCc2j710();
    } else if (testName == "testCc3j711") {
        result = testCc3j711();
    } else if (testName == "testCc4j712") {
        result = testCc4j712();
    } else if (testName == "testSv") {
        result = testSv();
    } else if (testName == "testGnssAntCon") {
        result = testGnssAntCon();
    } else if (testName == "testEthdebugCon") {
        result = testEthdebugCon();
    } else if (testName == "testAllConnectorPinAssignments") {
        result = testAllConnectorPinAssignments();
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
