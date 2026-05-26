/**
#include "lib_base.h"
 * @file clock_and_timing_lib.h
 * @brief Header file for ClockAndTimingTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef CLOCKANDTIMINGTESTING_H
#define CLOCKANDTIMINGTESTING_H
#include "lib_base.h"

/**
 * @class class ClockAndTimingTesting : public TestBase
 * @brief Class for testing clockandtimingtesting
 * @details This class provides methods to test various clockandtimingtesting functionalities.
 */
class ClockAndTimingTesting : public TestBase
{
  public: /**
           * @brief Constructor for ClockAndTimingTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function ClockAndTimingTesting
           * @return TestOutput with pass/fail and data
           */

    ClockAndTimingTesting();
    /**
     * @brief Constructor for ClockAndTimingTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~ClockAndTimingTesting();
    /**
     * @brief Constructor for ClockAndTimingTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the systemclockaccuracy functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testSystemClockAccuracy();
    /**
     * @brief Constructor for ClockAndTimingTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the pciereferenceclock functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPcieReferenceClock();
    /**
     * @brief Constructor for ClockAndTimingTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the usbreferenceclock functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testUsbReferenceClock();
    /**
     * @brief Constructor for ClockAndTimingTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the ethernetreferenceclock functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testEthernetReferenceClock();
    /**
     * @brief Constructor for ClockAndTimingTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the cameraclockoutput functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCameraClockOutput();
    /**
     * @brief Constructor for ClockAndTimingTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the audioclock functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAudioClock();
    /**
     * @brief Constructor for ClockAndTimingTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the plllockstatus functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPllLockStatus();
    /**
     * @brief Constructor for ClockAndTimingTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the clockjittermeasurements functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testClockJitterMeasurements();
    /**
     * @brief Constructor for ClockAndTimingTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the rtcaccuracy functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testRtcAccuracy();
    /**
     * @brief Constructor for ClockAndTimingTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the rtcbatterybackup functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testRtcBatteryBackup();
    /**
     * @brief Constructor for ClockAndTimingTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the rtcalarmfunctionality functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testRtcAlarmFunctionality();
    /**
     * @brief Constructor for ClockAndTimingTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the rtctimepersistence functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testRtcTimePersistence();

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

#endif // CLOCKANDTIMINGTESTING_H
