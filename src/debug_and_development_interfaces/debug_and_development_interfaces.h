/**
#include "lib_base.h"
 * @file debug_and_development_interfaces.h
 * @brief Header file for DebugAndDevelopmentInterfaces class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef DEBUGANDDEVELOPMENTINTERFACES_H
#define DEBUGANDDEVELOPMENTINTERFACES_H
#include "lib_base.h"

/**
 * @class DebugAndDevelopmentInterfaces
 * @brief Class for testing debug and development interfaces
 * @details This class provides methods to test various debug and development interfaces
 * such as JTAG, Serial Wire Debug, UART, Trace Port, Performance Counters, and Debug Authentication.
 */
class DebugAndDevelopmentInterfaces : public TestBase
{
  public:
    /**
     * @brief Default constructor for DebugAndDevelopmentInterfaces
     */
    /**
     * @brief Constructor for DebugAndDevelopmentInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Constructor for DebugAndDevelopmentInterfaces

     */
    DebugAndDevelopmentInterfaces();
    /**
     * @brief Destructor for DebugAndDevelopmentInterfaces
     */
    /**
     * @brief Constructor for DebugAndDevelopmentInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for DebugAndDevelopmentInterfaces

     */
    ~DebugAndDevelopmentInterfaces();

    /**
     * @brief Tests the JTAG interface functionality
     * @return true if the JTAG interface test passes, false otherwise
     */
    /**
     * @brief Constructor for DebugAndDevelopmentInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the jtaginterface functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testJtagInterface();
    /**
     * @brief Tests the Serial Wire Debug interface functionality
     * @return true if the Serial Wire Debug interface test passes, false otherwise
     */
    /**
     * @brief Constructor for DebugAndDevelopmentInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the serialwiredebug functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSerialWireDebug();
    /**
     * @brief Tests the Debug UART interface functionality
     * @return true if the Debug UART interface test passes, false otherwise
     */
    /**
     * @brief Constructor for DebugAndDevelopmentInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the debuguart functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDebugUart();
    /**
     * @brief Tests the Trace Port interface functionality
     * @return true if the Trace Port interface test passes, false otherwise
     */
    /**
     * @brief Constructor for DebugAndDevelopmentInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the traceport functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testTracePort();
    /**
     * @brief Tests the Performance Counters functionality
     * @return true if the Performance Counters test passes, false otherwise
     */
    /**
     * @brief Constructor for DebugAndDevelopmentInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the performancecounters functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testPerformanceCounters();
    /**
     * @brief Tests the Debug Authentication functionality
     * @return true if the Debug Authentication test passes, false otherwise
     */
    /**
     * @brief Constructor for DebugAndDevelopmentInterfaces
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the debugauthentication functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDebugAuthentication();

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

#endif // DEBUGANDDEVELOPMENTINTERFACES_H
