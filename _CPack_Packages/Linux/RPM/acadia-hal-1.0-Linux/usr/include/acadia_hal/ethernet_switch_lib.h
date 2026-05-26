/**
#include "lib_base.h"
 * @file ethernet_switch_lib.h
 * @brief Header file for EthernetSwitchTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef ETHERNETSWITCHTESTING_H
#define ETHERNETSWITCHTESTING_H
#include "lib_base.h"

/**
 * @class class EthernetSwitchTesting : public TestBase
 * @brief Class for testing ethernetswitchtesting
 * @details This class provides methods to test various ethernetswitchtesting functionalities.
 */
class EthernetSwitchTesting : public TestBase
{
  public: /**
           * @brief Constructor for EthernetSwitchTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function EthernetSwitchTesting
           * @return TestOutput with pass/fail and data
           */

    EthernetSwitchTesting();
    /**
     * @brief Constructor for EthernetSwitchTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~EthernetSwitchTesting();
    /**
     * @brief Constructor for EthernetSwitchTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the switchportfunctionality functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testSwitchPortFunctionality();
    /**
     * @brief Constructor for EthernetSwitchTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the switchforwardingtable functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSwitchForwardingTable();
    /**
     * @brief Constructor for EthernetSwitchTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the switchvlansupport functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSwitchVlanSupport();
    /**
     * @brief Constructor for EthernetSwitchTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the switchqos functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSwitchQos();
    /**
     * @brief Constructor for EthernetSwitchTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the switchportmirroring functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSwitchPortMirroring();
    /**
     * @brief Constructor for EthernetSwitchTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the switchstatistics functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSwitchStatistics();
    /**
     * @brief Constructor for EthernetSwitchTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the redundantswitchoperation functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testRedundantSwitchOperation();
    /**
     * @brief Constructor for EthernetSwitchTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the switchfailover functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSwitchFailover();

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

#endif // ETHERNETSWITCHTESTING_H
