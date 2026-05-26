/**
#include "lib_base.h"
 * @file watchdog_and_reset_lib.h
 * @brief Header file for WatchdogAndResetTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef WATCHDOGANDRESETTESTING_H
#define WATCHDOGANDRESETTESTING_H
#include "lib_base.h"

/**
 * @class class WatchdogAndResetTesting : public TestBase
 * @brief Class for testing watchdogandresettesting
 * @details This class provides methods to test various watchdogandresettesting functionalities.
 */
class WatchdogAndResetTesting : public TestBase
{
  public: /**
           * @brief Constructor for WatchdogAndResetTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function WatchdogAndResetTesting
           * @return TestOutput with pass/fail and data
           */

    WatchdogAndResetTesting();
    /**
     * @brief Constructor for WatchdogAndResetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~WatchdogAndResetTesting();
    /**
     * @brief Constructor for WatchdogAndResetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the watchdogtimerfunctionality functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testWatchdogTimerFunctionality();
    /**
     * @brief Constructor for WatchdogAndResetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the watchdogtimeoutconfiguration functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testWatchdogTimeoutConfiguration();
    /**
     * @brief Constructor for WatchdogAndResetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the watchdogresetgeneration functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testWatchdogResetGeneration();
    /**
     * @brief Constructor for WatchdogAndResetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the watchdoginterruptgeneration functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testWatchdogInterruptGeneration();
    /**
     * @brief Constructor for WatchdogAndResetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the poweronreset functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPoweronReset();
    /**
     * @brief Constructor for WatchdogAndResetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the hardwareresetbutton functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testHardwareResetButton();
    /**
     * @brief Constructor for WatchdogAndResetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the softwarereset functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSoftwareReset();
    /**
     * @brief Constructor for WatchdogAndResetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the watchdogreset functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testWatchdogReset();
    /**
     * @brief Constructor for WatchdogAndResetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the thermalreset functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testThermalReset();
    /**
     * @brief Constructor for WatchdogAndResetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the resetreasonlogging functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testResetReasonLogging();

    /**
     * @brief Get list of all available test names
     * @return Vector of test names
     */
    std::vector<std::string> getAvailableTests() override;

    /**
     * @brief Get list of all available test names
     * @return Vector of test names
     */

    /**
     * @brief Run a single test by name
     * @param testName Name of the test to run
     * @return Test result with timing
     */
    TestResult runSingleTest(const std::string& testName) override;

  protected:
    std::vector<TestResult> runAllTests() override;
};

#endif // WATCHDOGANDRESETTESTING_H
