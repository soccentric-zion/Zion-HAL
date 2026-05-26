/**
#include "lib_base.h"
 * @file system_memory_lib.h
 * @brief Header file for SystemMemoryTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef SYSTEMMEMORYTESTING_H
#define SYSTEMMEMORYTESTING_H
#include "lib_base.h"

/**
 * @class class SystemMemoryTesting : public TestBase
 * @brief Class for testing systemmemorytesting
 * @details This class provides methods to test various systemmemorytesting functionalities.
 */
class SystemMemoryTesting : public TestBase
{
  public: /**
           * @brief Constructor for SystemMemoryTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function SystemMemoryTesting
           * @return TestOutput with pass/fail and data
           */

    SystemMemoryTesting();
    /**
     * @brief Constructor for SystemMemoryTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~SystemMemoryTesting();
    /**
     * @brief Constructor for SystemMemoryTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the all64gbmemoryaccessibility functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testAll64gbMemoryAccessibility();
    /**
     * @brief Constructor for SystemMemoryTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the memorybanktesting functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMemoryBankTesting();
    /**
     * @brief Constructor for SystemMemoryTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the 256bitmemoryinterface functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput test256bitMemoryInterface();
    /**
     * @brief Constructor for SystemMemoryTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the memoryclockfrequency functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMemoryClockFrequency();
    /**
     * @brief Constructor for SystemMemoryTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the memorybandwidth functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMemoryBandwidth();
    /**
     * @brief Constructor for SystemMemoryTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the memorylatencymeasurements functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMemoryLatencyMeasurements();
    /**
     * @brief Constructor for SystemMemoryTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the memoryerrorcorrection functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMemoryErrorCorrection();
    /**
     * @brief Constructor for SystemMemoryTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the memorythermalbehavior functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMemoryThermalBehavior();
    /**
     * @brief Constructor for SystemMemoryTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the sequentialreadwriteperformance functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSequentialReadwritePerformance();
    /**
     * @brief Constructor for SystemMemoryTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the randomreadwriteperformance functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testRandomReadwritePerformance();
    /**
     * @brief Constructor for SystemMemoryTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the memoryallocationdeallocation functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMemoryAllocationdeallocation();
    /**
     * @brief Constructor for SystemMemoryTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the memoryfragmentationhandling functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMemoryFragmentationHandling();
    /**
     * @brief Constructor for SystemMemoryTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the outofmemoryconditions functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testOutofmemoryConditions();
    /**
     * @brief Constructor for SystemMemoryTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the memoryleaksdetection functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMemoryLeaksDetection();
    /**
     * @brief Constructor for SystemMemoryTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the dmatofrommemory functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDmaTofromMemory();
    /**
     * @brief Constructor for SystemMemoryTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the concurrentmemoryaccesspatterns functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testConcurrentMemoryAccessPatterns();

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

#endif // SYSTEMMEMORYTESTING_H
