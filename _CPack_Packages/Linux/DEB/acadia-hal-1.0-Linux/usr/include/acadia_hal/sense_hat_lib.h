/**
 * @file sense_hat_lib.h
 * @brief Header file for SenseHatTesting class
 * @author sandesh ghimire
 * @copyright (C) Lucid Motors */
#ifndef SENSEHATTESTING_H
#define SENSEHATTESTING_H
#include "lib_base.h"

/**
 * @class SenseHatTesting
 * @brief Tests for the Raspberry Pi Sense HAT (V1 / V2).
 *
 * @details The Sense HAT attaches to the Pi 5 40-pin header and provides:
 *   - 8×8 RGB LED matrix  (rpi-sense-fb framebuffer or I2C ATtiny88 @ 0x46)
 *   - Temperature / Humidity sensor  (HTS221, I2C 0x5C, IIO)
 *   - Barometric pressure sensor     (LPS25H, I2C 0x5F, IIO)
 *   - 9-DOF IMU (LSM9DS1): accel @ 0x1C, gyro @ 0x6A, mag @ 0x1E
 *   - 5-way mini joystick            (evdev input device)
 *
 * All tests use Linux sysfs / IIO / evdev interfaces; no proprietary library
 * is required.  Tests pass with an informational note if the HAT is absent.
 */
class SenseHatTesting : public TestBase
{
  public:
    SenseHatTesting();
    ~SenseHatTesting();

    /** @brief Detect Sense HAT on I2C bus (scans expected I2C addresses). */
    TestOutput testSenseHatDetection();

    /** @brief Verify 8×8 LED matrix framebuffer device exists and is writable. */
    TestOutput testLedMatrix();

    /** @brief Read ambient temperature from HTS221 via IIO subsystem. */
    TestOutput testTemperatureSensor();

    /** @brief Read relative humidity from HTS221 via IIO subsystem. */
    TestOutput testHumiditySensor();

    /** @brief Read barometric pressure from LPS25H via IIO subsystem. */
    TestOutput testPressureSensor();

    /** @brief Read accelerometer / gyroscope / magnetometer from LSM9DS1 via IIO. */
    TestOutput testImuSensors();

    /** @brief Detect 5-way joystick as an evdev input device. */
    TestOutput testJoystick();

    /** @brief Verify I2C bus used by the HAT (/dev/i2c-1) is accessible. */
    TestOutput testI2cBusIntegrity();

    std::vector<std::string> getAvailableTests() override;
    TestResult runSingleTest(const std::string& testName) override;

  protected:
    std::vector<TestResult> runAllTests() override;
};

#endif // SENSEHATTESTING_H
