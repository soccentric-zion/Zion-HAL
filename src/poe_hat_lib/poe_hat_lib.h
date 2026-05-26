/**
 * @file poe_hat_lib.h
 * @brief Header file for PoeHatTesting class
 * @author sandesh ghimire
 * @copyright (C) Lucid Motors */
#ifndef POEHATTESTING_H
#define POEHATTESTING_H
#include "lib_base.h"

/**
 * @class PoeHatTesting
 * @brief Tests for the Raspberry Pi PoE+ HAT (or third-party PoE HAT).
 *
 * @details The official Raspberry Pi PoE+ HAT provides:
 *   - IEEE 802.3at PoE+ (25.5 W) power delivery over Ethernet
 *   - Onboard 5V/5A DC-DC converter feeding the Pi via 4-pin header
 *   - Controllable brushless fan (PWM via ATtiny I2C controller @ 0x1A)
 *   - Current/voltage monitoring (optional on third-party HATs via INA219/INA226)
 *   - Fan speed feedback (tachometer)
 *
 * Tests use sysfs (hwmon, thermal, power_supply), I2C tools, and ethtool
 * to verify PoE negotiation, power delivery, and thermal management.
 */
class PoeHatTesting : public TestBase
{
  public:
    PoeHatTesting();
    ~PoeHatTesting();

    /** @brief Detect PoE HAT hardware (I2C controller + hwmon). */
    TestOutput testPoeHatDetection();

    /** @brief Verify PoE power negotiation via ethtool PoE tlv / sysfs. */
    TestOutput testPoePowerNegotiation();

    /** @brief Measure input voltage and current (INA219/INA226 hwmon if present). */
    TestOutput testPowerMonitoring();

    /** @brief Verify onboard fan presence and PWM control via sysfs hwmon. */
    TestOutput testFanControl();

    /** @brief Read fan tachometer (RPM) via hwmon. */
    TestOutput testFanSpeed();

    /** @brief Check thermal trip point that triggers fan speed increase. */
    TestOutput testThermalProtection();

    /** @brief Verify 5V rail stability via vcgencmd / power_supply sysfs. */
    TestOutput testVoltageRegulator();

    /** @brief End-to-end power delivery: verify Pi runs under load from PoE. */
    TestOutput testPowerDelivery();

    std::vector<std::string> getAvailableTests() override;
    TestResult runSingleTest(const std::string& testName) override;

  protected:
    std::vector<TestResult> runAllTests() override;
};

#endif // POEHATTESTING_H
