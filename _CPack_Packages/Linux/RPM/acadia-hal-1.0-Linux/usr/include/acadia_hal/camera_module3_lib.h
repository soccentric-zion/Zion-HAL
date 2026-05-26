/**
 * @file camera_module3_lib.h
 * @brief Header file for CameraModule3Testing class
 * @author sandesh ghimire
 * @copyright (C) Lucid Motors */
#ifndef CAMERAMODULE3TESTING_H
#define CAMERAMODULE3TESTING_H
#include "lib_base.h"

/**
 * @class CameraModule3Testing
 * @brief Tests for the Raspberry Pi Camera Module 3 (IMX708).
 *
 * @details The Camera Module 3 uses the Sony IMX708 sensor:
 *   - 12.3 MP (4608×2592), 1/2.43" sensor
 *   - Phase-detection autofocus (PDAF)
 *   - HDR mode (staggered dual-exposure)
 *   - FoV: standard 66° / wide 102° diagonal
 *   - Interface: MIPI CSI-2 2-lane D-PHY → RP1 Unicam
 *   - V4L2 driver: unicam + bcm2835-isp
 *   - libcamera pipeline: RPi (Raspberry Pi)
 */
class CameraModule3Testing : public TestBase
{
  public:
    CameraModule3Testing();
    ~CameraModule3Testing();

    /** @brief Detect IMX708 sensor via libcamera / media-ctl / V4L2 sysfs. */
    TestOutput testCameraDetection();

    /** @brief Verify sensor reports 4608×2592 maximum resolution. */
    TestOutput testSensorResolution();

    /** @brief Check PDAF (phase-detection autofocus) availability via libcamera controls. */
    TestOutput testAutofocus();

    /** @brief Verify HDR mode support (staggered capture / tonemapping). */
    TestOutput testHdrMode();

    /** @brief Attempt a single still capture via libcamera-still or v4l2-ctl. */
    TestOutput testStillCapture();

    /** @brief Verify MIPI CSI-2 interface via RP1 Unicam sysfs / dmesg. */
    TestOutput testMipiCsiInterface();

    /** @brief Check supported pixel formats (RAW10, RGB888, YUV420, etc.). */
    TestOutput testPixelFormats();

    /** @brief Verify camera video streaming at 1080p30 via libcamera-vid. */
    TestOutput testVideoStreaming();

    std::vector<std::string> getAvailableTests() override;
    TestResult runSingleTest(const std::string& testName) override;

  protected:
    std::vector<TestResult> runAllTests() override;
};

#endif // CAMERAMODULE3TESTING_H
