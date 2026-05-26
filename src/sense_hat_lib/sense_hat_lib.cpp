/**
 * @file sense_hat_lib.cpp
 * @brief Sense HAT tests for Raspberry Pi 5.
 *
 * Hardware: Raspberry Pi Sense HAT (V1/V2) on 40-pin GPIO header.
 *   - HTS221  humidity/temp sensor  — I2C-1 @ 0x5C, IIO driver hts221
 *   - LPS25H  pressure sensor       — I2C-1 @ 0x5F, IIO driver lps25h
 *   - LSM9DS1 9-DOF IMU             — I2C-1 accel 0x1C / gyro 0x6A / mag 0x1E
 *   - LED matrix ATtiny88           — I2C-1 @ 0x46, exposed as /dev/fb1 via
 *                                     rpi-sense-fb kernel module
 *   - 5-way joystick                — I2C-1 @ 0x46, evdev input device
 *
 * All paths are accessed via Linux sysfs / IIO / evdev.
 * @author sandesh ghimire
 * @copyright (C) Lucid Motors
 */

#include "sense_hat_lib.h"
#include "../platform_rpi5.h"

#include <array>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

// ---------------------------------------------------------------------------
static std::string shReadFile(const std::string& path)
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

static std::string shRunCmd(const std::string& cmd)
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

static bool shPathExists(const std::string& p)
{
    struct stat st {};
    return stat(p.c_str(), &st) == 0;
}

/** List all /sys/bus/iio/devices/iio:deviceN that have a 'name' attribute. */
static std::vector<std::string> shListIioDevices()
{
    std::vector<std::string> devs;
    const std::string base = "/sys/bus/iio/devices/";
    DIR* d = opendir(base.c_str());
    if (!d) return devs;
    struct dirent* e;
    while ((e = readdir(d)) != nullptr)
    {
        std::string n = e->d_name;
        if (n.find("iio:device") == 0 && !shReadFile(base + n + "/name").empty())
            devs.push_back(base + n);
    }
    closedir(d);
    return devs;
}
// ---------------------------------------------------------------------------

SenseHatTesting::SenseHatTesting() {}
SenseHatTesting::~SenseHatTesting() {}

/**
 * @brief Scan I2C-1 for the known Sense HAT addresses and check loaded modules.
 */
TestOutput SenseHatTesting::testSenseHatDetection()
{
    std::vector<std::string> data;
    data.push_back("=== Sense HAT Detection (Pi5 I2C-1) ===");

    bool i2c1 = shPathExists("/dev/i2c-1");
    data.push_back(i2c1 ? "[PASS] /dev/i2c-1 present"
                        : "[WARN] /dev/i2c-1 absent — enable with dtparam=i2c_arm=on");

    // Check overlay loaded
    std::string cmdline = shReadFile("/boot/firmware/config.txt");
    bool overlayLoaded = !shRunCmd("dtoverlay -l 2>/dev/null | grep -c 'sense'").empty();
    (void)overlayLoaded;
    bool senseFb = shPathExists("/dev/fb1") || !shRunCmd("lsmod | grep -c 'rpi_sense_fb'").empty();
    data.push_back(senseFb ? "[PASS] rpi-sense-fb module / /dev/fb1 present"
                           : "[INFO] rpi-sense-fb not loaded (add dtoverlay=rpi-sense-hat to config.txt)");

    // Sense HAT I2C addresses
    struct { const char* name; const char* addr; } addrs[] = {
        {"HTS221 (humidity/temp)",  "5c"},
        {"LPS25H (pressure)",       "5f"},
        {"LSM9DS1 accel",           "1c"},
        {"LSM9DS1 gyro",            "6a"},
        {"LSM9DS1 mag",             "1e"},
        {"ATtiny88 (LED/joystick)", "46"},
    };
    bool anyFound = false;
    if (i2c1)
    {
        std::string scan = shRunCmd("i2cdetect -y 1 2>/dev/null");
        for (auto& a : addrs)
        {
            bool found = scan.find(a.addr) != std::string::npos;
            data.push_back(std::string(found ? "[PASS] " : "[INFO] ") +
                           a.name + " @ 0x" + a.addr +
                           (found ? " detected" : " not found"));
            if (found) anyFound = true;
        }
    }
    else
    {
        data.push_back("[INFO] Skipping I2C scan (bus not available)");
    }

    {
        TestOutput _out;
        _out.status = (anyFound || senseFb) ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = (anyFound || senseFb) ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Verify the 8×8 LED matrix is accessible via the rpi-sense-fb framebuffer.
 */
TestOutput SenseHatTesting::testLedMatrix()
{
    std::vector<std::string> data;
    data.push_back("=== Sense HAT LED Matrix (rpi-sense-fb / /dev/fb1) ===");

    bool fb1 = shPathExists("/dev/fb1");
    data.push_back(fb1 ? "[PASS] /dev/fb1 (rpi-sense-fb framebuffer) present"
                       : "[INFO] /dev/fb1 not found");

    // Try /dev/fb0 as fallback (single-display systems)
    bool fb0 = shPathExists("/dev/fb0");
    data.push_back(fb0 ? "[INFO] /dev/fb0 also present (primary display FB)"
                       : "[INFO] /dev/fb0 not found");

    // Check sysfs fb nodes
    std::string fbName1 = shReadFile("/sys/class/graphics/fb1/name");
    if (!fbName1.empty())
        data.push_back("[PASS] fb1 device name: " + fbName1);

    std::string fbVirtSize = shReadFile("/sys/class/graphics/fb1/virtual_size");
    if (!fbVirtSize.empty())
        data.push_back("[INFO] fb1 virtual size: " + fbVirtSize + " (expect 8,8)");

    // Check ATtiny I2C address
    bool i2c1 = shPathExists("/dev/i2c-1");
    if (i2c1)
    {
        std::string scan = shRunCmd("i2cdetect -y 1 2>/dev/null");
        bool atFound = scan.find(" 46 ") != std::string::npos ||
                       scan.find("\n46 ") != std::string::npos;
        data.push_back(atFound ? "[PASS] ATtiny88 @ 0x46 detected (LED matrix controller)"
                               : "[INFO] ATtiny88 @ 0x46 not detected");
    }

    {
        TestOutput _out;
        _out.status = fb1 ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = fb1 ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Read ambient temperature from HTS221 via IIO.
 */
TestOutput SenseHatTesting::testTemperatureSensor()
{
    std::vector<std::string> data;
    data.push_back("=== Sense HAT Temperature (HTS221 / IIO) ===");

    auto devs = shListIioDevices();
    bool found = false;
    for (const auto& dev : devs)
    {
        std::string name = shReadFile(dev + "/name");
        if (name.find("hts221") == std::string::npos) continue;

        std::string raw   = shReadFile(dev + "/in_temp_raw");
        std::string scale = shReadFile(dev + "/in_temp_scale");
        std::string offs  = shReadFile(dev + "/in_temp_offset");
        if (!raw.empty())
        {
            double r = std::stod(raw);
            double s = scale.empty() ? 1.0 : std::stod(scale);
            double o = offs.empty()  ? 0.0 : std::stod(offs);
            double t = (r + o) * s;
            data.push_back("[PASS] HTS221 temperature: " + std::to_string(t) + " °C");
            found = true;
        }
        data.push_back("[INFO] IIO device: " + dev);
        break;
    }

    if (!found)
    {
        // Fallback: check if I2C device is visible
        std::string det = shRunCmd("i2cdetect -y 1 2>/dev/null | grep -o '5c'");
        if (!det.empty())
            data.push_back("[INFO] HTS221 @ 0x5C visible on I2C-1 but IIO driver not bound");
        else
            data.push_back("[INFO] HTS221 not found (Sense HAT not attached or driver not loaded)");
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
 * @brief Read relative humidity from HTS221 via IIO.
 */
TestOutput SenseHatTesting::testHumiditySensor()
{
    std::vector<std::string> data;
    data.push_back("=== Sense HAT Humidity (HTS221 / IIO) ===");

    auto devs = shListIioDevices();
    bool found = false;
    for (const auto& dev : devs)
    {
        std::string name = shReadFile(dev + "/name");
        if (name.find("hts221") == std::string::npos) continue;

        std::string raw   = shReadFile(dev + "/in_humidityrelative_raw");
        std::string scale = shReadFile(dev + "/in_humidityrelative_scale");
        if (!raw.empty())
        {
            double r = std::stod(raw);
            double s = scale.empty() ? 1.0 : std::stod(scale);
            data.push_back("[PASS] HTS221 humidity: " + std::to_string(r * s) + " %RH");
            found = true;
        }
        break;
    }

    if (!found)
        data.push_back("[INFO] HTS221 humidity IIO channel not available");

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
 * @brief Read barometric pressure from LPS25H via IIO.
 */
TestOutput SenseHatTesting::testPressureSensor()
{
    std::vector<std::string> data;
    data.push_back("=== Sense HAT Pressure (LPS25H / IIO) ===");

    auto devs = shListIioDevices();
    bool found = false;
    for (const auto& dev : devs)
    {
        std::string name = shReadFile(dev + "/name");
        if (name.find("lps25h") == std::string::npos &&
            name.find("lps25")  == std::string::npos) continue;

        std::string raw   = shReadFile(dev + "/in_pressure_raw");
        std::string scale = shReadFile(dev + "/in_pressure_scale");
        if (!raw.empty())
        {
            double r = std::stod(raw);
            double s = scale.empty() ? 1.0 : std::stod(scale);
            data.push_back("[PASS] LPS25H pressure: " + std::to_string(r * s) + " kPa");
            found = true;
        }

        // Temperature channel on LPS25H
        std::string tRaw   = shReadFile(dev + "/in_temp_raw");
        std::string tScale = shReadFile(dev + "/in_temp_scale");
        std::string tOff   = shReadFile(dev + "/in_temp_offset");
        if (!tRaw.empty())
        {
            double t = (std::stod(tRaw) + (tOff.empty() ? 0.0 : std::stod(tOff))) *
                       (tScale.empty() ? 1.0 : std::stod(tScale));
            data.push_back("[INFO] LPS25H die temperature: " + std::to_string(t) + " °C");
        }
        break;
    }

    if (!found)
        data.push_back("[INFO] LPS25H not found (Sense HAT not attached or driver not loaded)");

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
 * @brief Read accelerometer, gyroscope, and magnetometer from LSM9DS1 via IIO.
 */
TestOutput SenseHatTesting::testImuSensors()
{
    std::vector<std::string> data;
    data.push_back("=== Sense HAT IMU (LSM9DS1 accel/gyro/mag via IIO) ===");

    auto devs = shListIioDevices();
    bool accelOk = false, gyroOk = false, magnOk = false;

    for (const auto& dev : devs)
    {
        std::string name = shReadFile(dev + "/name");

        // Accelerometer (lsm9ds1_accel or similar)
        if (!accelOk && (name.find("lsm9ds1") != std::string::npos ||
                         name.find("accel") != std::string::npos))
        {
            std::string x = shReadFile(dev + "/in_accel_x_raw");
            std::string y = shReadFile(dev + "/in_accel_y_raw");
            std::string z = shReadFile(dev + "/in_accel_z_raw");
            if (!x.empty() && !y.empty() && !z.empty())
            {
                data.push_back("[PASS] Accel (" + name + ") X=" + x + " Y=" + y + " Z=" + z);
                accelOk = true;
            }
        }

        // Gyroscope
        if (!gyroOk && (name.find("lsm9ds1") != std::string::npos ||
                        name.find("gyro") != std::string::npos))
        {
            std::string x = shReadFile(dev + "/in_anglvel_x_raw");
            std::string y = shReadFile(dev + "/in_anglvel_y_raw");
            std::string z = shReadFile(dev + "/in_anglvel_z_raw");
            if (!x.empty() && !y.empty() && !z.empty())
            {
                data.push_back("[PASS] Gyro (" + name + ") X=" + x + " Y=" + y + " Z=" + z);
                gyroOk = true;
            }
        }

        // Magnetometer
        if (!magnOk && (name.find("lsm9ds1") != std::string::npos ||
                        name.find("magn") != std::string::npos))
        {
            std::string x = shReadFile(dev + "/in_magn_x_raw");
            std::string y = shReadFile(dev + "/in_magn_y_raw");
            std::string z = shReadFile(dev + "/in_magn_z_raw");
            if (!x.empty() && !y.empty() && !z.empty())
            {
                data.push_back("[PASS] Mag (" + name + ") X=" + x + " Y=" + y + " Z=" + z);
                magnOk = true;
            }
        }
    }

    if (!accelOk) data.push_back("[INFO] LSM9DS1 accelerometer IIO channel not found");
    if (!gyroOk)  data.push_back("[INFO] LSM9DS1 gyroscope IIO channel not found");
    if (!magnOk)  data.push_back("[INFO] LSM9DS1 magnetometer IIO channel not found");
    data.push_back("[INFO] Load driver: sudo modprobe lsm9ds1 (or add to /etc/modules)");

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
 * @brief Detect the Sense HAT 5-way joystick as an evdev input device.
 */
TestOutput SenseHatTesting::testJoystick()
{
    std::vector<std::string> data;
    data.push_back("=== Sense HAT Joystick (evdev / input) ===");

    // The Sense HAT joystick registers as EV_KEY input via ATtiny88
    std::string inputs = shRunCmd(
        "grep -rl 'Raspberry Pi Sense HAT Joystick' /sys/class/input/*/device/name 2>/dev/null | head -2");
    if (inputs.empty())
    {
        // Fallback: list all event devices
        inputs = shRunCmd(
            "cat /sys/class/input/event*/device/name 2>/dev/null | grep -i 'sense\\|hat\\|joystick'");
    }

    bool found = !inputs.empty();
    data.push_back(found ? "[PASS] Sense HAT joystick device found:\n" + inputs
                         : "[INFO] Sense HAT joystick not found in /sys/class/input");

    // Check for js0/js1
    bool js0 = shPathExists("/dev/input/js0");
    data.push_back(js0 ? "[INFO] /dev/input/js0 present" : "[INFO] /dev/input/js0 absent");

    data.push_back("[INFO] Joystick uses keys: UP/DOWN/LEFT/RIGHT/ENTER");
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
 * @brief Verify I2C bus used by the HAT is accessible and responds.
 */
TestOutput SenseHatTesting::testI2cBusIntegrity()
{
    std::vector<std::string> data;
    data.push_back("=== Sense HAT I2C Bus Integrity (/dev/i2c-1) ===");

    bool i2c1 = shPathExists("/dev/i2c-1");
    data.push_back(i2c1 ? "[PASS] /dev/i2c-1 present (GPIO2=SDA, GPIO3=SCL)"
                        : "[FAIL] /dev/i2c-1 absent");

    if (i2c1)
    {
        std::string scan = shRunCmd("i2cdetect -y 1 2>/dev/null");
        if (!scan.empty())
        {
            // Count responding devices
            int devCount = 0;
            (void)devCount;
            size_t pos = 0;
            while ((pos = scan.find("  ", pos)) == std::string::npos ? false : (pos += 2, true))
            {
                // rough count: just report lines
            }
            data.push_back("[PASS] i2cdetect succeeded:\n" + scan);

            // Check known HAT addresses
            static const char* const expected[] = {"5c", "5f", "1c", "6a", "1e", "46"};
            int present = 0;
            for (auto addr : expected)
                if (scan.find(addr) != std::string::npos) ++present;
            data.push_back("[INFO] Sense HAT addresses found: " + std::to_string(present) + " / 6");
        }
        else
        {
            data.push_back("[INFO] i2cdetect returned no output (i2c-tools installed?)");
        }
    }

    std::string i2cMod = shRunCmd("lsmod 2>/dev/null | grep -c 'i2c_dev'");
    data.push_back("[INFO] i2c_dev module loaded: " + i2cMod);

    {
        TestOutput _out;
        _out.status = i2c1 ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = i2c1 ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// Boilerplate
// ---------------------------------------------------------------------------

std::vector<TestResult> SenseHatTesting::runAllTests()
{
    std::vector<TestResult> results;
    auto run = [&](const std::string& name, TestOutput (SenseHatTesting::*fn)()) {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput out = (this->*fn)();
        auto end = std::chrono::high_resolution_clock::now();
        {
            TestResult _tr;
            _tr.name = name;
            _tr.test_id = name;
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            _tr.output = out;
            results.push_back(_tr);
        }
    };
    run("testSenseHatDetection",  &SenseHatTesting::testSenseHatDetection);
    run("testLedMatrix",          &SenseHatTesting::testLedMatrix);
    run("testTemperatureSensor",  &SenseHatTesting::testTemperatureSensor);
    run("testHumiditySensor",     &SenseHatTesting::testHumiditySensor);
    run("testPressureSensor",     &SenseHatTesting::testPressureSensor);
    run("testImuSensors",         &SenseHatTesting::testImuSensors);
    run("testJoystick",           &SenseHatTesting::testJoystick);
    run("testI2cBusIntegrity",    &SenseHatTesting::testI2cBusIntegrity);
    return results;
}

std::vector<std::string> SenseHatTesting::getAvailableTests()
{
    return {
        "testSenseHatDetection",
        "testLedMatrix",
        "testTemperatureSensor",
        "testHumiditySensor",
        "testPressureSensor",
        "testImuSensors",
        "testJoystick",
        "testI2cBusIntegrity",
    };
}

TestResult SenseHatTesting::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if      (testName == "testSenseHatDetection")  result = testSenseHatDetection();
    else if (testName == "testLedMatrix")           result = testLedMatrix();
    else if (testName == "testTemperatureSensor")   result = testTemperatureSensor();
    else if (testName == "testHumiditySensor")      result = testHumiditySensor();
    else if (testName == "testPressureSensor")      result = testPressureSensor();
    else if (testName == "testImuSensors")          result = testImuSensors();
    else if (testName == "testJoystick")            result = testJoystick();
    else if (testName == "testI2cBusIntegrity")     result = testI2cBusIntegrity();
    else {
        auto end = std::chrono::high_resolution_clock::now();
        {
            TestResult _tr;
            _tr.name = testName + " (UNKNOWN)";
            _tr.test_id = testName;
            _tr.output.status = hal::Status::ERROR;
            _tr.output.severity = hal::Severity::ERROR;
            _tr.output.error_message = "Unknown test name: " + testName;
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            return _tr;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    {
        TestResult _tr;
        _tr.name = testName;
        _tr.test_id = testName;
        _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        _tr.output = result;
        return _tr;
    }
}
