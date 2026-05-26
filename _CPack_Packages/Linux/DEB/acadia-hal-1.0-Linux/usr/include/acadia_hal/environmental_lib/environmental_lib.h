/**
#include "lib_base.h"
 * @file environmental_lib.h
 * @brief Header file for EnvironmentalTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef ENVIRONMENTALTESTING_H
#define ENVIRONMENTALTESTING_H
#include "lib_base.h"

/**
 * @class class EnvironmentalTesting : public TestBase
 * @brief Class for testing environmentaltesting
 * @details This class provides methods to test various environmentaltesting functionalities.
 */
class EnvironmentalTesting : public TestBase
{
  public: /**
           * @brief Constructor for EnvironmentalTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function EnvironmentalTesting
           * @return TestOutput with pass/fail and data
           */

    EnvironmentalTesting();
    /**
     * @brief Constructor for EnvironmentalTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~EnvironmentalTesting();
    /**
     * @brief Constructor for EnvironmentalTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the temperaturerange functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testTemperatureRange();
    /**
     * @brief Constructor for EnvironmentalTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the humidityresistance functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testHumidityResistance();
    /**
     * @brief Constructor for EnvironmentalTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the vibrationresistance functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testVibrationResistance();
    /**
     * @brief Constructor for EnvironmentalTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the shockresistance functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testShockResistance();
    /**
     * @brief Constructor for EnvironmentalTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the emiemccompliance functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testEmiemcCompliance();
    /**
     * @brief Constructor for EnvironmentalTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the esdprotection functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testEsdProtection();
    /**
     * @brief Constructor for EnvironmentalTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the saltsprayresistance functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSaltSprayResistance();

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

#endif // ENVIRONMENTALTESTING_H
