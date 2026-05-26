/**
#include "lib_base.h"
 * @file camera_interfaces_lib.h
 * @brief Header file for CameraInterfacesTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef CAMERAINTERFACESTESTING_H
#define CAMERAINTERFACESTESTING_H
#include "lib_base.h"

/**
 * @class class CameraInterfacesTesting
 * @brief Class for testing camerainterfacestesting
 * @details This class provides methods to test various camerainterfacestesting functionalities.
 */
class CameraInterfacesTesting : public TestBase
{
  public: /**
           * @brief Constructor for CameraInterfacesTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function CameraInterfacesTesting
           * @return TestOutput with pass/fail and data
           */

    CameraInterfacesTesting();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~CameraInterfacesTesting();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the allcsi2lanesfunctionality functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testAllCsi2LanesFunctionality();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the csi2dphyoperation functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCsi2DphyOperation();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the csi2cphyoperation functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCsi2CphyOperation();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the multiplecamerasimultaneousoperation functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMultipleCameraSimultaneousOperation();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the virtualchannelsupport functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testVirtualChannelSupport();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the cameraclockgeneration functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCameraClockGeneration();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the camerapowersequencing functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCameraPowerSequencing();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the raw8 functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testRaw8();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the raw10 functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testRaw10();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the raw12 functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testRaw12();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the raw14 functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testRaw14();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the yuv422 functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testYuv422();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the rgbformats functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testRgbFormats();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the isppipelineprocessing functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testIspPipelineProcessing();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the autoexposure functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAutoExposure();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the autowhitebalance functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAutoWhiteBalance();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the autofocus functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAutoFocus();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the lensshadingcorrection functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testLensShadingCorrection();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the demosaicing functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDemosaicing();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the noisereduction functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testNoiseReduction();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the edgeenhancement functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testEdgeEnhancement();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the colorcorrectionmatrix functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testColorCorrectionMatrix();
    /**
     * @brief Constructor for CameraInterfacesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gammacorrection functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGammaCorrection();

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

#endif // CAMERAINTERFACESTESTING_H
