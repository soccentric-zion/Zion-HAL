/**
#include "lib_base.h"
 * @file storage_lib.h
 * @brief Header file for StorageTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef STORAGETESTING_H
#define STORAGETESTING_H
#include "lib_base.h"

/**
 * @class class StorageTesting : public TestBase
 * @brief Class for testing storagetesting
 * @details This class provides methods to test various storagetesting functionalities.
 */
class StorageTesting : public TestBase
{
  public: /**
           * @brief Constructor for StorageTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function StorageTesting
           * @return TestOutput with pass/fail and data
           */

    StorageTesting();
    /**
     * @brief Constructor for StorageTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~StorageTesting();
    /**
     * @brief Constructor for StorageTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the emmc51interfacefunctionality functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testEmmc51InterfaceFunctionality();
    /**
     * @brief Constructor for StorageTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the fullcapacityverification functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testFullCapacityVerification();
    /**
     * @brief Constructor for StorageTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the sequentialreadperformance functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSequentialReadPerformance();
    /**
     * @brief Constructor for StorageTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the sequentialwriteperformance functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSequentialWritePerformance();
    /**
     * @brief Constructor for StorageTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the randomreadiops functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testRandomReadIops();
    /**
     * @brief Constructor for StorageTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the randomwriteiops functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testRandomWriteIops();
    /**
     * @brief Constructor for StorageTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the emmcwearleveling functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testEmmcWearLeveling();
    /**
     * @brief Constructor for StorageTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the badblockmanagement functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testBadBlockManagement();
    /**
     * @brief Constructor for StorageTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the powerlossprotection functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPowerLossProtection();
    /**
     * @brief Constructor for StorageTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the emmctemperaturemonitoring functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testEmmcTemperatureMonitoring();
    /**
     * @brief Constructor for StorageTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the nvmessddetection functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testNvmeSsdDetection();
    /**
     * @brief Constructor for StorageTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the nvmereadwriteperformance functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testNvmeReadwritePerformance();
    /**
     * @brief Constructor for StorageTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the nvmethermalthrottling functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testNvmeThermalThrottling();
    /**
     * @brief Constructor for StorageTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the hotplugcapability functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testHotplugCapability();
    /**
     * @brief Constructor for StorageTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the storageencryptionsupport functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testStorageEncryptionSupport();

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

#endif // STORAGETESTING_H
