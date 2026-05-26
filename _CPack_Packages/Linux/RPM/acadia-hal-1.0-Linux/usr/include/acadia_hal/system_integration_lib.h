/**
#include "lib_base.h"
 * @file system_integration_lib.h
 * @brief Header file for SystemIntegrationTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef SYSTEMINTEGRATIONTESTING_H
#define SYSTEMINTEGRATIONTESTING_H
#include "lib_base.h"

/**
 * @class class SystemIntegrationTesting : public TestBase
 * @brief Class for testing systemintegrationtesting
 * @details This class provides methods to test various systemintegrationtesting functionalities.
 */
class SystemIntegrationTesting : public TestBase
{
  public: /**
           * @brief Constructor for SystemIntegrationTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function SystemIntegrationTesting
           * @return TestOutput with pass/fail and data
           */

    SystemIntegrationTesting();
    /**
     * @brief Constructor for SystemIntegrationTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~SystemIntegrationTesting();
    /**
     * @brief Constructor for SystemIntegrationTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the concurrentoperationofallsubsystems functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testConcurrentOperationOfAllSubsystems();
    /**
     * @brief Constructor for SystemIntegrationTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the resourcecontentionhandling functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testResourceContentionHandling();
    /**
     * @brief Constructor for SystemIntegrationTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the systembottleneckidentification functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSystemBottleneckIdentification();
    /**
     * @brief Constructor for SystemIntegrationTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the endtoendlatency functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testEndtoendLatency();
    /**
     * @brief Constructor for SystemIntegrationTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the systemthroughput functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSystemThroughput();
    /**
     * @brief Constructor for SystemIntegrationTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the multimodalaiworkloads functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMultimodalAiWorkloads();
    /**
     * @brief Constructor for SystemIntegrationTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the autonomousdrivingstack functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAutonomousDrivingStack();
    /**
     * @brief Constructor for SystemIntegrationTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the sensorfusionpipeline functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSensorFusionPipeline();

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

#endif // SYSTEMINTEGRATIONTESTING_H
