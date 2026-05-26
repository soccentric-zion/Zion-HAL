/**
#include "lib_base.h"
 * @file gnss_lib.h
 * @brief Header file for GnssTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef GNSSTESTING_H
#define GNSSTESTING_H
#include "lib_base.h"

/**
 * @class class GnssTesting : public TestBase
 * @brief Class for testing gnsstesting
 * @details This class provides methods to test various gnsstesting functionalities.
 */
class GnssTesting : public TestBase
{
  public: /**
           * @brief Constructor for GnssTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function GnssTesting
           * @return TestOutput with pass/fail and data
           */

    GnssTesting();
    /**
     * @brief Constructor for GnssTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~GnssTesting();
    /**
     * @brief Constructor for GnssTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpssignalreception functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testGpsSignalReception();
    /**
     * @brief Constructor for GnssTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the glonasssupport functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGlonassSupport();
    /**
     * @brief Constructor for GnssTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the galileosupport functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGalileoSupport();
    /**
     * @brief Constructor for GnssTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the beidousupport functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testBeidouSupport();
    /**
     * @brief Constructor for GnssTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the positionaccuracy functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPositionAccuracy();
    /**
     * @brief Constructor for GnssTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the timetofirstfix functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testTimetofirstfix();
    /**
     * @brief Constructor for GnssTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gnssantennaconnection functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGnssAntennaConnection();
    /**
     * @brief Constructor for GnssTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the agpsfunctionality functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAgpsFunctionality();

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

#endif // GNSSTESTING_H
