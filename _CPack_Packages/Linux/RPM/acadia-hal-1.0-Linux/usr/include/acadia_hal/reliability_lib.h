/**
#include "lib_base.h"
 * @file reliability_lib.h
 * @brief Header file for ReliabilityTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef RELIABILITYTESTING_H
#define RELIABILITYTESTING_H
#include "lib_base.h"

/**
 * @class class ReliabilityTesting : public TestBase
 * @brief Class for testing reliabilitytesting
 * @details This class provides methods to test various reliabilitytesting functionalities.
 */
class ReliabilityTesting : public TestBase
{
  public: /**
           * @brief Constructor for ReliabilityTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function ReliabilityTesting
           * @return TestOutput with pass/fail and data
           */

    ReliabilityTesting();
    /**
     * @brief Constructor for ReliabilityTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~ReliabilityTesting();
    /**
     * @brief Constructor for ReliabilityTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the burnintesting functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testBurninTesting();
    /**
     * @brief Constructor for ReliabilityTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the powercycling functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPowerCycling();
    /**
     * @brief Constructor for ReliabilityTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the thermalcycling functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testThermalCycling();
    /**
     * @brief Constructor for ReliabilityTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the mtbf functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMtbf();
    /**
     * @brief Constructor for ReliabilityTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the componentaging functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testComponentAging();
    /**
     * @brief Constructor for ReliabilityTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the solderjointintegrity functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSolderJointIntegrity();

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

#endif // RELIABILITYTESTING_H
