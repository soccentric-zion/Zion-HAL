/**
#include "lib_base.h"
 * @file functional_safety_lib.h
 * @brief Header file for FunctionalSafetyTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef FUNCTIONALSAFETYTESTING_H
#define FUNCTIONALSAFETYTESTING_H
#include "lib_base.h"

/**
 * @class class FunctionalSafetyTesting : public TestBase
 * @brief Class for testing functionalsafetytesting
 * @details This class provides methods to test various functionalsafetytesting functionalities.
 */
class FunctionalSafetyTesting : public TestBase
{
  public: /**
           * @brief Constructor for FunctionalSafetyTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function FunctionalSafetyTesting
           * @return TestOutput with pass/fail and data
           */

    FunctionalSafetyTesting();
    /**
     * @brief Constructor for FunctionalSafetyTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~FunctionalSafetyTesting();
    /**
     * @brief Constructor for FunctionalSafetyTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the lockstepcpuoperation functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testLockstepCpuOperation();
    /**
     * @brief Constructor for FunctionalSafetyTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the ecconmemories functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testEccOnMemories();
    /**
     * @brief Constructor for FunctionalSafetyTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the redundantpowersupplies functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testRedundantPowerSupplies();
    /**
     * @brief Constructor for FunctionalSafetyTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the safetymonitoring functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSafetyMonitoring();
    /**
     * @brief Constructor for FunctionalSafetyTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the diagnosticcoverage functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDiagnosticCoverage();
    /**
     * @brief Constructor for FunctionalSafetyTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the faultinjectiontesting functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testFaultInjectionTesting();
    /**
     * @brief Constructor for FunctionalSafetyTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the failsafebehavior functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testFailsafeBehavior();
    /**
     * @brief Constructor for FunctionalSafetyTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the asilqslandrrailisolation functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAsilQsLAndRRailIsolation();

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

#endif // FUNCTIONALSAFETYTESTING_H
