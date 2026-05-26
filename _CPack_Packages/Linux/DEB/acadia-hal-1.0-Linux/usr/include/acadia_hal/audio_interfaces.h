/**
#include "lib_base.h"
 * @file audio_interfaces.h
 * @brief Header file for AudioInterfaces class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef AUDIOINTERFACES_H
#define AUDIOINTERFACES_H
#include "lib_base.h"

/**
 * @class class AudioInterfaces
 * @brief Class for testing audiointerfaces
 * @details This class provides methods to test various audiointerfaces functionalities.
 */
class AudioInterfaces : public TestBase
{
  public: /**
           * @brief Constructor for AudioInterfaces
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function AudioInterfaces
           * @return TestOutput with pass/fail and data
           */

    AudioInterfaces();
    /**
     * @brief Constructor for AudioInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~AudioInterfaces();
    /**
     * @brief Constructor for AudioInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the i2sinterface functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testI2sInterface();
    /**
     * @brief Constructor for AudioInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the audioinput functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAudioInput();
    /**
     * @brief Constructor for AudioInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the audiooutput functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAudioOutput();
    /**
     * @brief Constructor for AudioInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the audiosamplerates functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAudioSampleRates();
    /**
     * @brief Constructor for AudioInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the audiobitdepths functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAudioBitDepths();
    /**
     * @brief Constructor for AudioInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the multichannelaudio functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMultiChannelAudio();
    /**
     * @brief Constructor for AudioInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the audiodmatransfers functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAudioDmaTransfers();
    /**
     * @brief Constructor for AudioInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the audiolatency functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAudioLatency();
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

#endif // AUDIOINTERFACES_H
