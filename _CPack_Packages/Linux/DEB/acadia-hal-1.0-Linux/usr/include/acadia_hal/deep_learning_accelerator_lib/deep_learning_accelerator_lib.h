/**
#include "lib_base.h"
 * @file deep_learning_accelerator_lib.h
 * @brief Header file for DeepLearningAcceleratorTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef DEEPLEARNINGACCELERATORTESTING_H
#define DEEPLEARNINGACCELERATORTESTING_H
#include "lib_base.h"

/**
 * @class class DeepLearningAcceleratorTesting : public TestBase
 * @brief Class for testing deeplearningacceleratortesting
 * @details This class provides methods to test various deeplearningacceleratortesting functionalities.
 */
class DeepLearningAcceleratorTesting : public TestBase
{
  public: /**
           * @brief Constructor for DeepLearningAcceleratorTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function DeepLearningAcceleratorTesting
           * @return TestOutput with pass/fail and data
           */

    DeepLearningAcceleratorTesting();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~DeepLearningAcceleratorTesting();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the dla1functionality functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testDla1Functionality();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the dla2functionality functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDla2Functionality();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the dualdlaconcurrentoperation functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDualDlaConcurrentOperation();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the dlaclockfrequency functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDlaClockFrequency();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the dlapowerconsumption functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDlaPowerConsumption();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the convolutionoperations functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testConvolutionOperations();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the deconvolutionoperations functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDeconvolutionOperations();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the fullyconnectedlayers functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testFullyConnectedLayers();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the activationfunctions functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testActivationFunctions();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the poolingoperations functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPoolingOperations();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the batchnormalization functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testBatchNormalization();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the elementwiseoperations functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testElementwiseOperations();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the int8precision functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testInt8Precision();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the fp16precision functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testFp16Precision();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the structuredsparsitysupport functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testStructuredSparsitySupport();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the hardwarescheduleroperation functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testHardwareSchedulerOperation();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the dlatopsverification functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDlaTopsVerification();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the dlathroughputtesting functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDlaThroughputTesting();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the dlalatencymeasurements functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDlaLatencyMeasurements();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the dlamemorybandwidth functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDlaMemoryBandwidth();
    /**
     * @brief Constructor for DeepLearningAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the dlathermalbehavior functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDlaThermalBehavior();

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

#endif // DEEPLEARNINGACCELERATORTESTING_H
