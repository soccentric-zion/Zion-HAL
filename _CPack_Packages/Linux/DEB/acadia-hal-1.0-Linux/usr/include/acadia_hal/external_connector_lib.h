/**
#include "lib_base.h"
 * @file external_connector_lib.h
 * @brief Header file for ExternalConnectorTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef EXTERNALCONNECTORTESTING_H
#define EXTERNALCONNECTORTESTING_H
#include "lib_base.h"

/**
 * @class class ExternalConnectorTesting : public TestBase
 * @brief Class for testing externalconnectortesting
 * @details This class provides methods to test various externalconnectortesting functionalities.
 */
class ExternalConnectorTesting : public TestBase
{
  public: /**
           * @brief Constructor for ExternalConnectorTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function ExternalConnectorTesting
           * @return TestOutput with pass/fail and data
           */

    ExternalConnectorTesting();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~ExternalConnectorTesting();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the kl15powerinput functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testKl15PowerInput();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the adenlrsignals functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAdenlrSignals();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the bcmcaninterface functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testBcmCanInterface();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the vcucaninterface functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testVcuCanInterface();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the allconnectorpinscontinuity functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAllConnectorPinsContinuity();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the connectorretentionforce functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testConnectorRetentionForce();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the connectorcontactresistance functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testConnectorContactResistance();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the 12vasilqsrpower functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput test12vAsilQsRPower();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the connectormechanicalintegrity functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testConnectorMechanicalIntegrity();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the etcl3j703 functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testEtcl3j703();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the etcl2j704 functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testEtcl2j704();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the etc3j705 functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testEtc3j705();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the allethernetphyconnections functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAllEthernetPhyConnections();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the cc1j709 functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCc1j709();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the cc2j710 functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCc2j710();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the cc3j711 functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCc3j711();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the cc4j712 functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCc4j712();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the sv functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSv();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gnssantcon functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGnssAntCon();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the ethdebugcon functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testEthdebugCon();
    /**
     * @brief Constructor for ExternalConnectorTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the allconnectorpinassignments functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAllConnectorPinAssignments();

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

#endif // EXTERNALCONNECTORTESTING_H
