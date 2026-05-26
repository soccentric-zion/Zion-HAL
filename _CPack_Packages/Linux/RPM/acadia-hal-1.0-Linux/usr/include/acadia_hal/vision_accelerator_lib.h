/**
#include "lib_base.h"
 * @file vision_accelerator_lib.h
 * @brief Header file for VisionAcceleratorTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef VISIONACCELERATORTESTING_H
#define VISIONACCELERATORTESTING_H
#include "lib_base.h"

/**
 * @class class VisionAcceleratorTesting : public TestBase
 * @brief Class for testing visionacceleratortesting
 * @details This class provides methods to test various visionacceleratortesting functionalities.
 */
class VisionAcceleratorTesting : public TestBase
{
  public: /**
           * @brief Constructor for VisionAcceleratorTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function VisionAcceleratorTesting
           * @return TestOutput with pass/fail and data
           */

    VisionAcceleratorTesting();
    /**
     * @brief Constructor for VisionAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~VisionAcceleratorTesting();
    /**
     * @brief Constructor for VisionAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the dualvectorprocessingunits functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testDualVectorProcessingUnits();
    /**
     * @brief Constructor for VisionAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the vectorcoresineachvpu functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testVectorCoresInEachVpu();
    /**
     * @brief Constructor for VisionAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the vliwslots functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testVliwSlots();
    /**
     * @brief Constructor for VisionAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the scalarexecutionunits functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testScalarExecutionUnits();
    /**
     * @brief Constructor for VisionAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the vectorexecutionunits functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testVectorExecutionUnits();
    /**
     * @brief Constructor for VisionAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the dmaengine1functionality functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDmaEngine1Functionality();
    /**
     * @brief Constructor for VisionAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the dmaengine2functionality functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDmaEngine2Functionality();
    /**
     * @brief Constructor for VisionAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the 5dimensionaladdressing functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput test5dimensionalAddressing();
    /**
     * @brief Constructor for VisionAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the 16hardwarechannelsperengine functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput test16HardwareChannelsPerEngine();
    /**
     * @brief Constructor for VisionAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the dmabandwidthtesting functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDmaBandwidthTesting();
    /**
     * @brief Constructor for VisionAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the dmalatencytesting functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDmaLatencyTesting();
    /**
     * @brief Constructor for VisionAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the computervisionalgorithms functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testComputerVisionAlgorithms();
    /**
     * @brief Constructor for VisionAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the imageprocessingpipelines functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testImageProcessingPipelines();
    /**
     * @brief Constructor for VisionAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the featuredetection functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testFeatureDetection();
    /**
     * @brief Constructor for VisionAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the opticalflow functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testOpticalFlow();
    /**
     * @brief Constructor for VisionAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the stereodisparity functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testStereoDisparity();
    /**
     * @brief Constructor for VisionAcceleratorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the pvapowerconsumption functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPvaPowerConsumption();

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

#endif // VISIONACCELERATORTESTING_H
