/**
#include "lib_base.h"
 * @file imu_lib.h
 * @brief Header file for ImuTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef IMUTESTING_H
#define IMUTESTING_H
#include "lib_base.h"

/**
 * @class class ImuTesting : public TestBase
 * @brief Class for testing imutesting
 * @details This class provides methods to test various imutesting functionalities.
 */
class ImuTesting : public TestBase
{
  public: /**
           * @brief Constructor for ImuTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function ImuTesting
           * @return TestOutput with pass/fail and data
           */

    ImuTesting();
    /**
     * @brief Constructor for ImuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~ImuTesting();
    /**
     * @brief Constructor for ImuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the 3axisaccelerometer functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput test3axisAccelerometer();
    /**
     * @brief Constructor for ImuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the 3axisgyroscope functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput test3axisGyroscope();
    /**
     * @brief Constructor for ImuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the 3axismagnetometer functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput test3axisMagnetometer();
    /**
     * @brief Constructor for ImuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the imudatarate functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testImuDataRate();
    /**
     * @brief Constructor for ImuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the imuaccuracy functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testImuAccuracy();
    /**
     * @brief Constructor for ImuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the imucalibration functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testImuCalibration();
    /**
     * @brief Constructor for ImuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the imutemperaturecompensation functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testImuTemperatureCompensation();
    /**
     * @brief Constructor for ImuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the imutoorininterface functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testImuToOrinInterface();

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

#endif // IMUTESTING_H
