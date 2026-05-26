/**
#include "lib_base.h"
 * @file thermal_management_lib.h
 * @brief Header file for ThermalManagementTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef THERMALMANAGEMENTTESTING_H
#define THERMALMANAGEMENTTESTING_H
#include "lib_base.h"

/**
 * @class class ThermalManagementTesting : public TestBase
 * @brief Class for testing thermalmanagementtesting
 * @details This class provides methods to test various thermalmanagementtesting functionalities.
 */
class ThermalManagementTesting : public TestBase
{
  public: /**
           * @brief Constructor for ThermalManagementTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function ThermalManagementTesting
           * @return TestOutput with pass/fail and data
           */

    ThermalManagementTesting();
    /**
     * @brief Constructor for ThermalManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~ThermalManagementTesting();
    /**
     * @brief Constructor for ThermalManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the socjunctiontemperature functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testSocJunctionTemperature();
    /**
     * @brief Constructor for ThermalManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gputemperature functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpuTemperature();
    /**
     * @brief Constructor for ThermalManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the cputemperature functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCpuTemperature();
    /**
     * @brief Constructor for ThermalManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the memorytemperature functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMemoryTemperature();
    /**
     * @brief Constructor for ThermalManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the boardambienttemperature functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testBoardAmbientTemperature();
    /**
     * @brief Constructor for ThermalManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the allthermalzones functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAllThermalZones();
    /**
     * @brief Constructor for ThermalManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the thermalthrottlingthresholds functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testThermalThrottlingThresholds();
    /**
     * @brief Constructor for ThermalManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the thermalshutdowntemperature functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testThermalShutdownTemperature();
    /**
     * @brief Constructor for ThermalManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the coolingsolutioneffectiveness functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCoolingSolutionEffectiveness();
    /**
     * @brief Constructor for ThermalManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the temperaturedistributionacrossdie functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testTemperatureDistributionAcrossDie();
    /**
     * @brief Constructor for ThermalManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the sustainedthermalperformance functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSustainedThermalPerformance();
    /**
     * @brief Constructor for ThermalManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the thermalrecoverytime functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testThermalRecoveryTime();
    /**
     * @brief Constructor for ThermalManagementTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the fancontrol functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testFanControl();

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

#endif // THERMALMANAGEMENTTESTING_H
