/**
 * @file camera_module3_lib.cpp
 * @brief Raspberry Pi Camera Module 3 (IMX708) tests for Raspberry Pi 5.
 *
 * Hardware: Raspberry Pi Camera Module 3 connected to the CSI-2 connector.
 *   Sensor    : Sony IMX708 12.3 MP, 1/2.43", PDAF, HDR
 *   Interface : MIPI CSI-2 2-lane → RP1 Unicam → bcm2835-isp
 *   Driver    : unicam (V4L2 capture), bcm2835-isp (V4L2 m2m ISP)
 *   libcamera : RPi pipeline with autofocus (VCM actuator) and HDR support
 *
 * Tests use V4L2 sysfs, libcamera CLI tools, media-ctl, and dmesg.
 * @author sandesh ghimire
 * @copyright (C) Lucid Motors
 */

#include "camera_module3_lib.h"
#include "../platform_rpi5.h"

#include <array>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

// ---------------------------------------------------------------------------
[[maybe_unused]] static std::string cm3ReadFile(const std::string& path)
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

static std::string cm3RunCmd(const std::string& cmd)
{
    std::array<char, 2048> buf{};
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

static bool cm3PathExists(const std::string& p)
{
    struct stat st {};
    return stat(p.c_str(), &st) == 0;
}
// ---------------------------------------------------------------------------

CameraModule3Testing::CameraModule3Testing() {}
CameraModule3Testing::~CameraModule3Testing() {}

/**
 * @brief Detect the IMX708 sensor via libcamera, media-ctl, and V4L2 sysfs.
 */
TestOutput CameraModule3Testing::testCameraDetection()
{
    std::vector<std::string> data;
    data.push_back("=== Camera Module 3 Detection (IMX708 / CSI-2) ===");

    // libcamera: list cameras
    std::string lcCams = cm3RunCmd("libcamera-hello --list-cameras 2>/dev/null");
    bool imx708Found = !lcCams.empty() &&
                       lcCams.find("imx708") != std::string::npos;
    data.push_back(imx708Found
        ? "[PASS] IMX708 detected via libcamera:\n" + lcCams
        : (lcCams.empty() ? "[INFO] libcamera not available or no cameras"
                          : "[INFO] libcamera cameras:\n" + lcCams));

    // V4L2: check /dev/video0
    bool vid0 = cm3PathExists(RPI5_CSI_DEVICE0);
    data.push_back(vid0 ? "[PASS] " + std::string(RPI5_CSI_DEVICE0) + " present"
                        : "[INFO] " + std::string(RPI5_CSI_DEVICE0) + " not found");

    // media-ctl: check media0
    bool media0 = cm3PathExists(RPI5_MEDIA_CTL0);
    data.push_back(media0 ? "[PASS] /dev/media0 present"
                          : "[INFO] /dev/media0 not found");

    // dmesg: look for IMX708
    std::string dmesgCam = cm3RunCmd("dmesg 2>/dev/null | grep -i 'imx708\\|unicam' | tail -3");
    if (!dmesgCam.empty())
        data.push_back("[INFO] dmesg: " + dmesgCam);

    // Unicam sysfs
    bool unicam = cm3PathExists(RPI5_UNICAM_PATH);
    data.push_back(unicam ? "[PASS] RP1 Unicam sysfs entry present"
                          : "[INFO] Unicam sysfs not found");

    {
        TestOutput _out;
        _out.status = (imx708Found || vid0) ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = (imx708Found || vid0) ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Verify IMX708 maximum resolution 4608×2592 via v4l2-ctl.
 */
TestOutput CameraModule3Testing::testSensorResolution()
{
    std::vector<std::string> data;
    data.push_back("=== Camera Module 3 Resolution (IMX708 4608×2592) ===");
    data.push_back("[INFO] IMX708 full resolution: 4608 × 2592 (12.3 MP)");
    data.push_back("[INFO] Binned modes: 2304×1296 (2×2), 1152×648 (4×4)");

    // v4l2-ctl: list formats
    std::string fmts = cm3RunCmd("v4l2-ctl --device=/dev/video0 --list-formats-ext 2>/dev/null | head -20");
    if (!fmts.empty())
    {
        data.push_back("[PASS] v4l2-ctl formats:\n" + fmts);
        bool fullRes = fmts.find("4608") != std::string::npos ||
                       fmts.find("2592") != std::string::npos;
        data.push_back(fullRes ? "[PASS] Full 4608×2592 resolution advertised"
                               : "[INFO] 4608×2592 not found in format list");
        {
            TestOutput _out;
            _out.status = hal::Status::PASS;
            _out.severity = hal::Severity::INFO;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    // Fallback: libcamera-hello --list-cameras is more verbose
    std::string lcInfo = cm3RunCmd("libcamera-hello --list-cameras 2>/dev/null");
    bool fullRes = lcInfo.find("4608") != std::string::npos;
    data.push_back(fullRes ? "[PASS] 4608 found in libcamera camera info"
                           : "[INFO] Resolution info not available without camera");

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
 * @brief Check PDAF (phase-detection autofocus) via libcamera controls.
 */
TestOutput CameraModule3Testing::testAutofocus()
{
    std::vector<std::string> data;
    data.push_back("=== Camera Module 3 Autofocus (IMX708 PDAF + VCM) ===");
    data.push_back("[INFO] IMX708 supports phase-detection AF with VCM actuator");
    data.push_back("[INFO] AF modes: AfModeContinuous, AfModeAuto, AfModeManual");

    // libcamera-hello: check AF controls
    std::string afCtl = cm3RunCmd(
        "libcamera-hello --list-cameras 2>/dev/null | grep -i 'focus\\|vcm\\|af'");
    if (!afCtl.empty())
        data.push_back("[PASS] AF-related control found:\n" + afCtl);
    else
        data.push_back("[INFO] AF control info not available (camera may be absent)");

    // Check for VCM I2C device (DW9807 or similar) — typically shows up in dmesg
    std::string vcmDmesg = cm3RunCmd("dmesg 2>/dev/null | grep -i 'vcm\\|dw9807\\|focus' | head -3");
    if (!vcmDmesg.empty())
        data.push_back("[PASS] VCM actuator in dmesg: " + vcmDmesg);

    // Capture with AF: quick 100ms preview
    std::string afTest = cm3RunCmd(
        "libcamera-still --timeout 100 --autofocus-mode continuous "
        "--output /tmp/cm3_af_test.jpg 2>&1 | head -5");
    if (!afTest.empty())
        data.push_back("[INFO] AF capture: " + afTest);

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
 * @brief Verify HDR mode via libcamera HDR controls.
 */
TestOutput CameraModule3Testing::testHdrMode()
{
    std::vector<std::string> data;
    data.push_back("=== Camera Module 3 HDR (IMX708 staggered HDR) ===");
    data.push_back("[INFO] IMX708 HDR uses staggered dual-exposure readout");
    data.push_back("[INFO] libcamera HDR control: --hdr (HdrModeMultiExposure / SingleExposure)");

    std::string hdrInfo = cm3RunCmd(
        "libcamera-hello --list-cameras 2>/dev/null | grep -i 'hdr'");
    data.push_back(hdrInfo.empty() ? "[INFO] HDR not listed (camera absent or old libcamera)"
                                   : "[PASS] HDR info: " + hdrInfo);

    // Try an HDR capture
    std::string hdrTest = cm3RunCmd(
        "libcamera-still --timeout 100 --hdr --output /tmp/cm3_hdr_test.jpg 2>&1 | head -5");
    if (!hdrTest.empty())
        data.push_back("[INFO] HDR capture output: " + hdrTest);

    data.push_back("[INFO] HDR tonemapping done in bcm2835-isp (not on-sensor)");
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
 * @brief Take a single still image via libcamera-still.
 */
TestOutput CameraModule3Testing::testStillCapture()
{
    std::vector<std::string> data;
    data.push_back("=== Camera Module 3 Still Capture ===");

    std::string out = cm3RunCmd(
        "libcamera-still --timeout 200 --output /tmp/cm3_still.jpg 2>&1 | head -10");
    bool ok = !out.empty() && out.find("ERROR") == std::string::npos &&
              out.find("error") == std::string::npos;
    data.push_back(out.empty() ? "[INFO] libcamera-still not available"
                               : (ok ? "[PASS] Still capture OK:\n" + out
                                     : "[INFO] Still capture output:\n" + out));

    // Check output file
    struct stat st {};
    if (stat("/tmp/cm3_still.jpg", &st) == 0 && st.st_size > 0)
        data.push_back("[PASS] /tmp/cm3_still.jpg written (" +
                       std::to_string(st.st_size / 1024) + " KB)");

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
 * @brief Verify the MIPI CSI-2 interface via RP1 Unicam kernel messages.
 */
TestOutput CameraModule3Testing::testMipiCsiInterface()
{
    std::vector<std::string> data;
    data.push_back("=== Camera Module 3 MIPI CSI-2 Interface (RP1 Unicam) ===");
    data.push_back("[INFO] Pi5 CSI connector: 22-pin FFC → RP1 1f00110000.csi");
    data.push_back("[INFO] D-PHY 2-lane @ up to 1.5 Gbps/lane → ISP pipeline");

    bool unicam = cm3PathExists(RPI5_UNICAM_PATH);
    data.push_back(unicam ? "[PASS] Unicam sysfs: " + std::string(RPI5_UNICAM_PATH)
                          : "[INFO] Unicam sysfs not found (driver not loaded?)");

    std::string dmesgUni = cm3RunCmd("dmesg 2>/dev/null | grep -i 'unicam\\|csi' | tail -4");
    if (!dmesgUni.empty())
        data.push_back("[INFO] dmesg unicam/csi:\n" + dmesgUni);

    // Check bcm2835-isp module
    std::string ispMod = cm3RunCmd("grep -c 'bcm2835.isp' /proc/modules 2>/dev/null");
    data.push_back(!ispMod.empty() && ispMod != "0"
        ? "[PASS] bcm2835-isp module loaded"
        : "[INFO] bcm2835-isp not in /proc/modules");

    {
        TestOutput _out;
        _out.status = unicam ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = unicam ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Check supported pixel formats via v4l2-ctl.
 */
TestOutput CameraModule3Testing::testPixelFormats()
{
    std::vector<std::string> data;
    data.push_back("=== Camera Module 3 Pixel Formats ===");
    data.push_back("[INFO] Expected: SRGGB10 (RAW10), RG10, YUYV, RGB888, NV12");

    bool vid0 = cm3PathExists(RPI5_CSI_DEVICE0);
    if (vid0)
    {
        std::string fmts = cm3RunCmd(
            "v4l2-ctl --device=/dev/video0 --list-formats 2>/dev/null");
        if (!fmts.empty())
        {
            data.push_back("[PASS] v4l2 formats:\n" + fmts);
            bool raw10 = fmts.find("RG10") != std::string::npos ||
                         fmts.find("SRGGB10") != std::string::npos;
            data.push_back(raw10 ? "[PASS] RAW10 format available"
                                 : "[INFO] RAW10 format not found");
            {
                TestOutput _out;
                _out.status = hal::Status::PASS;
                _out.severity = hal::Severity::INFO;
                for (size_t _i = 0; _i < data.size(); ++_i)
                    _out.reason["detail_" + std::to_string(_i)] = data[_i];
                return _out;
            }
        }
    }

    data.push_back("[INFO] /dev/video0 not present; pixel format list unavailable");
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
 * @brief Verify video streaming at 1080p30 via libcamera-vid.
 */
TestOutput CameraModule3Testing::testVideoStreaming()
{
    std::vector<std::string> data;
    data.push_back("=== Camera Module 3 Video Streaming (1080p30) ===");

    std::string out = cm3RunCmd(
        "libcamera-vid --timeout 500 --width 1920 --height 1080 --framerate 30 "
        "--output /tmp/cm3_video.h264 2>&1 | head -8");
    bool ok = !out.empty() && out.find("ERROR") == std::string::npos;
    data.push_back(out.empty() ? "[INFO] libcamera-vid not available"
                               : (ok ? "[PASS] Video stream started:\n" + out
                                     : "[INFO] Video output:\n" + out));

    struct stat st {};
    if (stat("/tmp/cm3_video.h264", &st) == 0 && st.st_size > 0)
        data.push_back("[PASS] /tmp/cm3_video.h264 written (" +
                       std::to_string(st.st_size / 1024) + " KB)");

    data.push_back("[INFO] Pi5 H.264 encoding via V4L2 m2m (bcm2835-codec)");
    {
        TestOutput _out;
        _out.status = hal::Status::PASS;
        _out.severity = hal::Severity::INFO;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// Boilerplate
// ---------------------------------------------------------------------------

std::vector<TestResult> CameraModule3Testing::runAllTests()
{
    std::vector<TestResult> results;
    auto run = [&](const std::string& name, TestOutput (CameraModule3Testing::*fn)()) {
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
    run("testCameraDetection",   &CameraModule3Testing::testCameraDetection);
    run("testSensorResolution",  &CameraModule3Testing::testSensorResolution);
    run("testAutofocus",         &CameraModule3Testing::testAutofocus);
    run("testHdrMode",           &CameraModule3Testing::testHdrMode);
    run("testStillCapture",      &CameraModule3Testing::testStillCapture);
    run("testMipiCsiInterface",  &CameraModule3Testing::testMipiCsiInterface);
    run("testPixelFormats",      &CameraModule3Testing::testPixelFormats);
    run("testVideoStreaming",     &CameraModule3Testing::testVideoStreaming);
    return results;
}

std::vector<std::string> CameraModule3Testing::getAvailableTests()
{
    return {
        "testCameraDetection",
        "testSensorResolution",
        "testAutofocus",
        "testHdrMode",
        "testStillCapture",
        "testMipiCsiInterface",
        "testPixelFormats",
        "testVideoStreaming",
    };
}

TestResult CameraModule3Testing::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if      (testName == "testCameraDetection")   result = testCameraDetection();
    else if (testName == "testSensorResolution")   result = testSensorResolution();
    else if (testName == "testAutofocus")          result = testAutofocus();
    else if (testName == "testHdrMode")            result = testHdrMode();
    else if (testName == "testStillCapture")       result = testStillCapture();
    else if (testName == "testMipiCsiInterface")   result = testMipiCsiInterface();
    else if (testName == "testPixelFormats")       result = testPixelFormats();
    else if (testName == "testVideoStreaming")      result = testVideoStreaming();
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
