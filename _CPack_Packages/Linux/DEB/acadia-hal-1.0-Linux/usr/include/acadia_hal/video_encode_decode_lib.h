/**
#include "lib_base.h"
 * @file video_encode_decode_lib.h
 * @brief Header file for VideoEncodeDecodeTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef VIDEOENCODEDECODETESTING_H
#define VIDEOENCODEDECODETESTING_H
#include "lib_base.h"

/**
 * @class class VideoEncodeDecodeTesting : public TestBase
 * @brief Class for testing videoencodedecodetesting
 * @details This class provides methods to test various videoencodedecodetesting functionalities.
 */
class VideoEncodeDecodeTesting : public TestBase
{
  public: /**
           * @brief Constructor for VideoEncodeDecodeTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function VideoEncodeDecodeTesting
           * @return TestOutput with pass/fail and data
           */

    VideoEncodeDecodeTesting();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~VideoEncodeDecodeTesting();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the h265encoding functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testH265Encoding();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the h264encoding functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testH264Encoding();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the av1encoding functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAv1Encoding();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the multiplestreamencoding functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMultipleStreamEncoding();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the variousresolutions functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testVariousResolutions();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the variousbitrates functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testVariousBitrates();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the variousframerates functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testVariousFrameRates();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the encoderqualitysettings functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testEncoderQualitySettings();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the encoderlatency functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testEncoderLatency();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the encoderpowerconsumption functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testEncoderPowerConsumption();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the h265decoding functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testH265Decoding();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the h264decoding functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testH264Decoding();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the av1decoding functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAv1Decoding();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the vp9decoding functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testVp9Decoding();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the multiplestreamdecoding functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMultipleStreamDecoding();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the 4k8kdecoding functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput test4k8kDecoding();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the decoderlatency functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDecoderLatency();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the corruptstreamhandling functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCorruptStreamHandling();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the jpegcompression functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testJpegCompression();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the jpegdecompression functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testJpegDecompression();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the imagescalingoperations functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testImageScalingOperations();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the yuvformatconversions functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testYuvFormatConversions();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the colorspaceconversions functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testColorSpaceConversions();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the jpegqualitylevels functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testJpegQualityLevels();
    /**
     * @brief Constructor for VideoEncodeDecodeTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the nvjpgthroughput functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testNvjpgThroughput();

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

#endif // VIDEOENCODEDECODETESTING_H
