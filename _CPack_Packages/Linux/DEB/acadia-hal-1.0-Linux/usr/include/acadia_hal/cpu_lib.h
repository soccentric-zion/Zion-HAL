/**
 * @file cpu_lib.h
 * @brief Header file for CpuTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef CPUTESTING_H
#define CPUTESTING_H

#include "lib_base.h"

/**
 * @class class CpuTesting
 * @brief Class for testing cputesting
 * @details This class provides methods to test various cputesting functionalities.
 */
class CpuTesting : public TestBase
{
  public: /**
           * @brief Constructor for CpuTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function CpuTesting
           * @return TestOutput with pass/fail and data
           */

    CpuTesting();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~CpuTesting();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the all12cpucoresfunctionality functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testAll12CpuCoresFunctionality();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the cpucorefrequencyscaling functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCpuCoreFrequencyScaling();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the individualcorestresstesting functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testIndividualCoreStressTesting();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the multicoresynchronization functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMulticoreSynchronization();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the cpuclustercommunication functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCpuClusterCommunication();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the coreaffinityandthreadmigration functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCoreAffinityAndThreadMigration();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the cputhermalthrottling functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCpuThermalThrottling();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the cpupowerstatesandtransitions functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCpuPowerStatesAndTransitions();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the l1cache functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testL1Cache();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the l2cache functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testL2Cache();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the l3cache functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testL3Cache();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the systemcache functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSystemCache();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the cachecoherencybetweenclusters functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCacheCoherencyBetweenClusters();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the cachelineinvalidation functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCacheLineInvalidation();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the cachebandwidthandlatency functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCacheBandwidthAndLatency();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the singlethreadedperformance functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSinglethreadedPerformance();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the multithreadedperformance functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMultithreadedPerformance();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the contextswitchingperformance functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testContextSwitchingPerformance();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the interruptlatency functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testInterruptLatency();
    /**
     * @brief Constructor for CpuTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the dmaoperations functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDmaOperations();

    /**
     * @brief Get list of all available test names
     * @return Vector of test names
     */
    std::vector<std::string> getAvailableTests() override;

    /**
     * @brief Run all CPU tests
     * @return Vector of test results with timing
     */
    std::vector<TestResult> runAllTests() override;

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
};

#endif // CPUTESTING_H
