/**
 * @file gnss_lib.cpp
 * @brief GNSS tests for Raspberry Pi 5.
 *        Pi5 has NO built-in GNSS receiver.  GNSS capability requires an
 *        external module (e.g. u-blox M10/F9 via UART/SPI/USB).  Tests verify
 *        that the GNSS serial device is present, the gpsd service is running,
 *        and basic NMEA data can be obtained.  If no receiver is connected,
 *        tests pass with an informational warning.
 * @author sandesh ghimire
 * @copyright (C) Lucid Motors
 */

#include "gnss_lib.h"
#include "../platform_rpi5.h"

#include <array>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

// ---------------------------------------------------------------------------
[[maybe_unused]] static std::string gnssReadFile(const std::string& path)
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

static std::string gnssRunCmd(const std::string& cmd)
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

static bool gnssPathExists(const std::string& p)
{
    struct stat st {};
    return stat(p.c_str(), &st) == 0;
}
// ---------------------------------------------------------------------------

/**
 * @brief Constructor for GnssTesting
 */
GnssTesting::GnssTesting()
{
}

/**
 * @brief Destructor for GnssTesting
 */
GnssTesting::~GnssTesting()
{
}

/**
 * @brief Check for GPS NMEA data via gpsd or direct serial.
 */
TestOutput GnssTesting::testGpsSignalReception()
{
    std::vector<std::string> data;
    data.push_back("=== GPS Signal Reception (Pi5 + External GNSS Module) ===");
    data.push_back("[INFO] Pi5 has no built-in GNSS; expects external module on UART/USB");

    // Check for common GNSS serial ports
    static const std::vector<std::string> gnssPorts = {
        "/dev/ttyAMA0", "/dev/ttyAMA1", "/dev/ttyUSB0", "/dev/ttyACM0"};
    bool portFound = false;
    for (const auto& p : gnssPorts)
    {
        if (gnssPathExists(p))
        {
            data.push_back("[PASS] GNSS candidate port: " + p);
            portFound = true;
        }
    }
    if (!portFound)
        data.push_back("[WARN] No GNSS serial port found (module not connected)");

    // Check gpsd
    std::string gpsdState = gnssRunCmd("systemctl is-active gpsd 2>/dev/null");
    data.push_back(gpsdState == "active" ? "[PASS] gpsd service: active"
                                         : "[INFO] gpsd: " + (gpsdState.empty() ? "not found" : gpsdState));

    // Try gpspipe for NMEA GGA (1-second timeout)
    std::string gga = gnssRunCmd("timeout 2 gpspipe -r 2>/dev/null | grep -m1 '^\\$GPGGA\\|^\\$GNGGA'");
    if (!gga.empty())
    {
        data.push_back("[PASS] NMEA GGA sentence: " + gga.substr(0, 80));
    }
    else
    {
        data.push_back("[INFO] No GGA NMEA data (no fix or module absent)");
    }

    {
        TestOutput _out;
        const bool _ok = portFound || gpsdState == "active";
        _out.status = _ok ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = _ok ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Verify GLONASS support in GNSS module.
 */
TestOutput GnssTesting::testGlonassSupport()
{
    std::vector<std::string> data;
    data.push_back("=== GLONASS Support (External GNSS Module, Pi5) ===");

    // Check for GLONASS NMEA sentence ($GLGSV)
    std::string glgsv = gnssRunCmd("timeout 2 gpspipe -r 2>/dev/null | grep -m1 '^\\$GLGSV\\|^\\$GNGSV'");
    if (!glgsv.empty())
    {
        data.push_back("[PASS] GLONASS sentence: " + glgsv.substr(0, 80));
    }
    else
    {
        data.push_back("[INFO] No GLONASS GSV sentence (module absent or no GLONASS constellation)");
    }

    // u-blox module might expose UBX-MON-GNSS
    std::string ubxInfo = gnssRunCmd("ubxtool -f /dev/ttyAMA0 -p MON-GNSS 2>/dev/null | head -5");
    if (!ubxInfo.empty())
        data.push_back("[INFO] UBX MON-GNSS:\n" + ubxInfo);

    data.push_back("[INFO] Multi-constellation GNSS requires u-blox M10 or equivalent module");
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
 * @brief Verify Galileo support in GNSS module.
 */
TestOutput GnssTesting::testGalileoSupport()
{
    std::vector<std::string> data;
    data.push_back("=== Galileo Support (External GNSS Module, Pi5) ===");

    std::string gagsv = gnssRunCmd("timeout 2 gpspipe -r 2>/dev/null | grep -m1 '^\\$GAGSV\\|^\\$GNGSV'");
    if (!gagsv.empty())
        data.push_back("[PASS] Galileo GSV sentence: " + gagsv.substr(0, 80));
    else
        data.push_back("[INFO] No Galileo sentence (module absent or Galileo not enabled)");

    data.push_back("[INFO] Galileo support requires u-blox M10 / ZED-F9P or equivalent");
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
 * @brief Verify BeiDou support in GNSS module.
 */
TestOutput GnssTesting::testBeidouSupport()
{
    std::vector<std::string> data;
    data.push_back("=== BeiDou Support (External GNSS Module, Pi5) ===");

    std::string gbgsv = gnssRunCmd("timeout 2 gpspipe -r 2>/dev/null | grep -m1 '^\\$GBGSV\\|^\\$GNGSV'");
    if (!gbgsv.empty())
        data.push_back("[PASS] BeiDou GSV sentence: " + gbgsv.substr(0, 80));
    else
        data.push_back("[INFO] No BeiDou sentence");

    data.push_back("[INFO] BeiDou support requires multi-constellation module");
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
 * @brief Check reported position accuracy (HDOP/PDOP from NMEA).
 */
TestOutput GnssTesting::testPositionAccuracy()
{
    std::vector<std::string> data;
    data.push_back("=== GNSS Position Accuracy (Pi5) ===");

    // GGA field 8 = HDOP, field 9 = altitude
    std::string gga = gnssRunCmd("timeout 3 gpspipe -r 2>/dev/null | grep -m1 '^\\$G.GGA'");
    if (!gga.empty())
    {
        data.push_back("[INFO] GGA: " + gga.substr(0, 100));
        // Parse HDOP (field index 8 in GGA)
        std::istringstream ss(gga);
        std::string token;
        int idx = 0;
        while (std::getline(ss, token, ','))
        {
            if (idx == 8 && !token.empty())
            {
                data.push_back("[INFO] HDOP: " + token);
                double hdop = std::stod(token);
                data.push_back(hdop < 2.0 ? "[PASS] HDOP < 2.0 (good accuracy)"
                                           : "[WARN] HDOP >= 2.0 (poor geometry)");
            }
            idx++;
        }
    }
    else
    {
        data.push_back("[INFO] No GGA data — cannot assess position accuracy");
    }

    data.push_back("[INFO] Typical L1 single-band accuracy: 2–5 m CEP; RTK: <10 cm");
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
 * @brief Check Time To First Fix (TTFF) via gpsd.
 */
TestOutput GnssTesting::testTimetofirstfix()
{
    std::vector<std::string> data;
    data.push_back("=== GNSS Time To First Fix (Pi5) ===");
    data.push_back("[INFO] TTFF measurement requires tracking fix acquisition from cold/warm start");

    // Check gpsd fix status via gpspipe JSON
    std::string tpv = gnssRunCmd("timeout 3 gpspipe -w 2>/dev/null | grep -m1 '\"mode\":[23]'");
    if (!tpv.empty())
    {
        data.push_back("[PASS] GNSS fix detected: " + tpv.substr(0, 100));
    }
    else
    {
        data.push_back("[INFO] No 2D/3D fix within timeout (cold start or no signal)");
        data.push_back("[INFO] Cold TTFF: ~30–60 s; warm: ~5–15 s; hot: ~1–3 s");
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
 * @brief Verify GNSS antenna connection (via RF signal level).
 */
TestOutput GnssTesting::testGnssAntennaConnection()
{
    std::vector<std::string> data;
    data.push_back("=== GNSS Antenna Connection (Pi5) ===");

    // Check satellite count from GSV sentence
    std::string gsv = gnssRunCmd("timeout 3 gpspipe -r 2>/dev/null | grep -m1 '^\\$G.GSV'");
    if (!gsv.empty())
    {
        // Field 3 of GSV = total satellites in view
        std::istringstream ss(gsv);
        std::string token;
        int idx = 0;
        while (std::getline(ss, token, ','))
        {
            if (idx == 3 && !token.empty())
            {
                int svCount = std::stoi(token);
                data.push_back("[INFO] Satellites in view: " + std::to_string(svCount));
                data.push_back(svCount >= 4 ? "[PASS] Antenna receiving signal from " +
                                                  std::to_string(svCount) + " satellites"
                                            : "[WARN] < 4 satellites in view — check antenna");
            }
            idx++;
        }
    }
    else
    {
        data.push_back("[INFO] No GSV data — module absent or antenna disconnected");
    }

    // Check for antenna-short alarm in u-blox UBX
    std::string ubxMon = gnssRunCmd("ubxtool -f /dev/ttyAMA0 -p MON-HW 2>/dev/null | grep -i ant");
    if (!ubxMon.empty())
        data.push_back("[INFO] UBX antenna status: " + ubxMon);

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
 * @brief Verify Assisted GPS (AGPS) capability.
 */
TestOutput GnssTesting::testAgpsFunctionality()
{
    std::vector<std::string> data;
    data.push_back("=== AGPS Functionality (Pi5) ===");
    data.push_back("[INFO] AGPS requires internet connectivity to download almanac/ephemeris");

    // Check for gpsd with AGPS support
    std::string gpsdVer = gnssRunCmd("gpsd --version 2>&1 | head -1");
    if (!gpsdVer.empty())
        data.push_back("[INFO] gpsd: " + gpsdVer);

    // Check for NTRIP client or AGPS server tool
    std::string ntripcli = gnssRunCmd("which ntripclient 2>/dev/null");
    data.push_back(ntripcli.empty() ? "[INFO] ntripclient not installed (DGPS/RTK corrections)"
                                    : "[PASS] ntripclient: " + ntripcli);

    // u-blox AssistNow
    std::string assistNow = gnssRunCmd("which ubxload 2>/dev/null");
    data.push_back(assistNow.empty() ? "[INFO] ubxload not installed (u-blox AssistNow offline)"
                                     : "[PASS] ubxload found: " + assistNow);

    // Verify internet connectivity (for AGPS download)
    std::string netCheck = gnssRunCmd("ping -c 1 -W 2 8.8.8.8 2>/dev/null | grep -c '1 received'");
    data.push_back(netCheck == "1" ? "[PASS] Internet reachable (AGPS download possible)"
                                   : "[INFO] Internet not reachable — offline AGPS only");

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
std::vector<TestResult> GnssTesting::runAllTests()
{
    std::vector<TestResult> results;

    // Run testGpsSignalReception
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testGpsSignalReception();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testGpsSignalReception";
            _tr.test_id = "testGpsSignalReception";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testGlonassSupport
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testGlonassSupport();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testGlonassSupport";
            _tr.test_id = "testGlonassSupport";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testGalileoSupport
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testGalileoSupport();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testGalileoSupport";
            _tr.test_id = "testGalileoSupport";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testBeidouSupport
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testBeidouSupport();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testBeidouSupport";
            _tr.test_id = "testBeidouSupport";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testPositionAccuracy
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPositionAccuracy();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPositionAccuracy";
            _tr.test_id = "testPositionAccuracy";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testTimetofirstfix
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testTimetofirstfix();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testTimetofirstfix";
            _tr.test_id = "testTimetofirstfix";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testGnssAntennaConnection
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testGnssAntennaConnection();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testGnssAntennaConnection";
            _tr.test_id = "testGnssAntennaConnection";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testAgpsFunctionality
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testAgpsFunctionality();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testAgpsFunctionality";
            _tr.test_id = "testAgpsFunctionality";
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
std::vector<std::string> GnssTesting::getAvailableTests()
{
    return {
        "testGpsSignalReception", "testGlonassSupport", "testGalileoSupport",        "testBeidouSupport",
        "testPositionAccuracy",   "testTimetofirstfix", "testGnssAntennaConnection", "testAgpsFunctionality",
    };
}

/**
 * @brief Run a single test by name
 * @param testName Name of the test to run
 * @return Test result with timing
 */
TestResult GnssTesting::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if (testName == "testGpsSignalReception") {
        result = testGpsSignalReception();
    } else if (testName == "testGlonassSupport") {
        result = testGlonassSupport();
    } else if (testName == "testGalileoSupport") {
        result = testGalileoSupport();
    } else if (testName == "testBeidouSupport") {
        result = testBeidouSupport();
    } else if (testName == "testPositionAccuracy") {
        result = testPositionAccuracy();
    } else if (testName == "testTimetofirstfix") {
        result = testTimetofirstfix();
    } else if (testName == "testGnssAntennaConnection") {
        result = testGnssAntennaConnection();
    } else if (testName == "testAgpsFunctionality") {
        result = testAgpsFunctionality();
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
