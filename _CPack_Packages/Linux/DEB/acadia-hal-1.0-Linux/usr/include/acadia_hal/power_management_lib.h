/**
#include "lib_base.h"
 * @file power_management_lib.h
 * @brief Header file for PowerManagementTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef POWERMANAGEMENTTESTING_H
#define POWERMANAGEMENTTESTING_H
#include "lib_base.h"

/**
 * @class class PowerManagementTesting : public TestBase
 * @brief Class for testing powermanagementtesting
 * @details This class provides methods to test various powermanagementtesting functionalities.
 */
class PowerManagementTesting : public TestBase
{
  public: /**
           * @brief Constructor for PowerManagementTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function PowerManagementTesting
           * @return TestOutput with pass/fail and data
           */

    PowerManagementTesting();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~PowerManagementTesting();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the maximumperformancemode functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testMaximumPerformanceMode();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the poweroptimizedmodes functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPoweroptimizedModes();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the idlepowerconsumption functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testIdlePowerConsumption();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the sleepstates functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSleepStates();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the powerstatetransitions functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPowerStateTransitions();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the wakeupsources functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testWakeupSources();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the powerbuttonfunctionality functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPowerButtonFunctionality();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the 12vasilqslrail functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput test12vAsilQsLRail();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the 12vasilqsrrail functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput test12vAsilQsRRail();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the allinternalvoltagerails functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAllInternalVoltageRails();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the powersequencing functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPowerSequencing();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the poweronreset functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPoweronReset();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the brownoutdetection functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testBrownoutDetection();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the overvoltageprotection functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testOvervoltageProtection();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the undervoltageprotection functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testUndervoltageProtection();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the overcurrentprotection functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testOvercurrentProtection();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the powersupplyrejection functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPowerSupplyRejection();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the realtimepowerconsumption functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testRealtimePowerConsumption();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the percomponentpowermeasurement functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPercomponentPowerMeasurement();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the powerefficiencycalculation functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPowerEfficiencyCalculation();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the batterycharging functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testBatteryCharging();
    /**
     * @brief Constructor for PowerManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the upsfunctionality functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testUpsFunctionality();

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

#endif // POWERMANAGEMENTTESTING_H
