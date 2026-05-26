/**
#include "lib_base.h"
 * @file ethernet_lib.h
 * @brief Header file for EthernetTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef ETHERNETTESTING_H
#define ETHERNETTESTING_H
#include "lib_base.h"

/**
 * @class class EthernetTesting : public TestBase
 * @brief Class for testing ethernettesting
 * @details This class provides methods to test various ethernettesting functionalities.
 */
class EthernetTesting : public TestBase
{
  public: /**
           * @brief Constructor for EthernetTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function EthernetTesting
           * @return TestOutput with pass/fail and data
           */

    EthernetTesting();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~EthernetTesting();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the 10gigabitlinkestablishment functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput test10GigabitLinkEstablishment();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the 10gbetransmitthroughput functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput test10gbeTransmitThroughput();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the 10gbereceivethroughput functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput test10gbeReceiveThroughput();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the fullduplexoperation functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testFullduplexOperation();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the jumboframessupport functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testJumboFramesSupport();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the flowcontrol functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testFlowControl();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the vlantagging functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testVlanTagging();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the checksumoffload functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testChecksumOffload();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the ieee8021astimesynchronization functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testIeee8021asTimeSynchronization();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the ieee8021qbv functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testIeee8021qbv();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the ieee8021qbu functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testIeee8021qbu();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the ieee8021qav functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testIeee8021qav();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the tsntrafficscheduling functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testTsnTrafficScheduling();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the tsnlatencyguarantees functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testTsnLatencyGuarantees();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the tsntimeprecision functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testTsnTimePrecision();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the 1gphyfunctionality functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput test1gPhyFunctionality();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the linkspeednegotiation functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testLinkSpeedNegotiation();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the automdimdix functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAutomdimdix();
    /**
     * @brief Constructor for EthernetTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the ethernetcablediagnostics functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testEthernetCableDiagnostics();

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

#endif // ETHERNETTESTING_H
