/**
 * @file pcie_lib.h
 * @brief Header file for PcieTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef PCIETESTING_H
#define PCIETESTING_H
#include "lib_base.h"

/**
 * @class PcieTesting : public TestBase
 * @brief Class for testing PCIe functionality
 * @details This class provides methods to test various PCIe functionalities.
 */
class PcieTesting : public TestBase
{
  public:
    /**
     * @brief Constructor for PcieTesting
     */
    PcieTesting();

    /**
     * @brief Destructor for PcieTesting
     */
    ~PcieTesting();

    /**
     * @brief Tests PCIe link training
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPcieLinkTraining();

    /**
     * @brief Tests PCIe link speed
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPcieLinkSpeed();

    /**
     * @brief Tests PCIe link width
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPcieLinkWidth();

    /**
     * @brief Tests PCIe lane reversal
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPcieLaneReversal();

    /**
     * @brief Tests PCIe hotplug support
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPcieHotplugSupport();

    /**
     * @brief Tests PCIe read bandwidth
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPcieReadBandwidth();

    /**
     * @brief Tests PCIe write bandwidth
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPcieWriteBandwidth();

    /**
     * @brief Tests PCIe latency
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPcieLatency();

    /**
     * @brief Tests PCIe error handling
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPcieErrorHandling();

    /**
     * @brief Tests PCIe power management
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPciePowerManagement();

    /**
     * @brief Tests secondary PCIe link training
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSecondaryPcieLinkTraining();

    /**
     * @brief Tests PCIe x4 bandwidth
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPcieX4Bandwidth();

    /**
     * @brief Tests PCIe x4 lane functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPcieX4LaneFunctionality();

    /**
     * @brief Tests concurrent PCIe x8 and x4 operation
     * @return TestOutput with pass/fail and data
     */
    TestOutput testConcurrentPcieX8AndX4Operation();

    /**
     * @brief Tests NVMe SSD connectivity
     * @return TestOutput with pass/fail and data
     */
    TestOutput testNvmeSsdConnectivity();

    /**
     * @brief Tests GPU add-in card support
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpuAddinCardSupport();

    /**
     * @brief Tests network adapter connectivity
     * @return TestOutput with pass/fail and data
     */
    TestOutput testNetworkAdapterConnectivity();

    /**
     * @brief Tests multiple PCIe device enumeration
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMultiplePcieDeviceEnumeration();

    /**
     * @brief Tests PCIe switch compatibility
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPcieSwitchCompatibility();

    /**
     * @brief Get list of all available test names
     * @return Vector of test names
     */
    std::vector<std::string> getAvailableTests() override;

    /**
     * @brief Run a single test by name
     * @param testName Name of the test to run
     * @return Test result with timing
     */
    TestResult runSingleTest(const std::string& testName) override;

  protected:
    /**
     * @brief Run all tests for this module
     * @return Vector of test results
     */
    std::vector<TestResult> runAllTests() override;

  private:
    /**
     * @brief Helper function to read from sysfs
     * @param path The sysfs path to read
     * @return The content as string
     */
    std::string readSysfsFile(const std::string& path);

    /**
     * @brief Helper function to get PCIe devices
     * @return Vector of PCIe device paths
     */
    std::vector<std::string> getPcieDevices();

    /**
     * @brief Helper function to execute command and get output
     * @param cmd The command to execute
     * @return The output as string
     */
    std::string executeCommand(const std::string& cmd);
};

#endif // PCIETESTING_H
