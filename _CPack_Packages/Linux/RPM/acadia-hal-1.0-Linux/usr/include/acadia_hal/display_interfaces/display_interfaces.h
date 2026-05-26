/**
#include "lib_base.h"
 * @file display_interfaces.h
 * @brief Header file for DisplayInterfaces class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef DISPLAYINTERFACES_H
#define DISPLAYINTERFACES_H
#include "lib_base.h"

/**
 * @class class DisplayInterfaces
 * @brief Class for testing displayinterfaces
 * @details This class provides methods to test various displayinterfaces functionalities.
 */
class DisplayInterfaces : public TestBase
{
  public: /**
           * @brief Constructor for DisplayInterfaces
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function DisplayInterfaces
           * @return TestOutput with pass/fail and data
           */

    DisplayInterfaces();
    /**
     * @brief Constructor for DisplayInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~DisplayInterfaces();
    /**
     * @brief Constructor for DisplayInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the displayportoutput functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testDisplayPortOutput();
    /**
     * @brief Constructor for DisplayInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the hdmioutput functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testHdmiOutput();
    /**
     * @brief Constructor for DisplayInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the multidisplaysupport functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMultiDisplaySupport();
    /**
     * @brief Constructor for DisplayInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the variousresolutions functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testVariousResolutions();
    /**
     * @brief Constructor for DisplayInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the variousrefreshrates functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testVariousRefreshRates();
    /**
     * @brief Constructor for DisplayInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the hdrsupport functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testHdrSupport();
    /**
     * @brief Constructor for DisplayInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the displayhotplugdetection functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDisplayHotPlugDetection();

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

#endif // DISPLAYINTERFACES_H
