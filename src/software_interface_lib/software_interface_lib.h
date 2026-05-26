/**
 * @file software_interface_lib.h
 * @brief Header file for SoftwareInterfaceTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef SOFTWAREINTERFACETESTING_H
#define SOFTWAREINTERFACETESTING_H

#include "lib_base.h"

/**
 * @class SoftwareInterfaceTesting
 * @brief Class for testing software interface functionalities
 * @details This class provides methods to test various software interface functionalities.
 */
class SoftwareInterfaceTesting : public TestBase
{
  public:
    /**
     * @brief Constructor for SoftwareInterfaceTesting
     */
    SoftwareInterfaceTesting();

    /**
     * @brief Destructor for SoftwareInterfaceTesting
     */
    ~SoftwareInterfaceTesting();

    /**
     * @brief Tests Linux kernel boot
     * @return TestOutput with pass/fail and data
     */
    TestOutput testLinuxKernelBoot();

    /**
     * @brief Tests device driver loading
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDeviceDriverLoading();

    /**
     * @brief Tests system service startup
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSystemServiceStartup();

    /**
     * @brief Tests application framework
     * @return TestOutput with pass/fail and data
     */
    TestOutput testApplicationFramework();

    /**
     * @brief Tests multiuser support
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMultiuserSupport();

    /**
     * @brief Tests CUDA runtime
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCudaRuntime();

    /**
     * @brief Tests cuDNN library
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCudnnLibrary();

    /**
     * @brief Tests TensorRT inference
     * @return TestOutput with pass/fail and data
     */
    TestOutput testTensorrtInference();

    /**
     * @brief Tests VPI
     * @return TestOutput with pass/fail and data
     */
    TestOutput testVpi();

    /**
     * @brief Tests multimedia API
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMultimediaApi();

    /**
     * @brief Tests sensor driver framework
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSensorDriverFramework();

    /**
     * @brief Get list of all available test names
     * @return Vector of test names
     */
    std::vector<std::string> getAvailableTests() override;

    /**
     * @brief Run all software interface tests
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

#endif // SOFTWAREINTERFACETESTING_H
