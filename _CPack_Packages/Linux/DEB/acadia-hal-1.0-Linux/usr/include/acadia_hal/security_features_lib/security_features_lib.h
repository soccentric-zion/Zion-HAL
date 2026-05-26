/**
#include "lib_base.h"
 * @file security_features_lib.h
 * @brief Header file for SecurityFeaturesTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef SECURITYFEATURESTESTING_H
#define SECURITYFEATURESTESTING_H
#include "lib_base.h"

/**
 * @class class SecurityFeaturesTesting : public TestBase
 * @brief Class for testing securityfeaturestesting
 * @details This class provides methods to test various securityfeaturestesting functionalities.
 */
class SecurityFeaturesTesting : public TestBase
{
  public: /**
           * @brief Constructor for SecurityFeaturesTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function SecurityFeaturesTesting
           * @return TestOutput with pass/fail and data
           */

    SecurityFeaturesTesting();
    /**
     * @brief Constructor for SecurityFeaturesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~SecurityFeaturesTesting();
    /**
     * @brief Constructor for SecurityFeaturesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the bootromexecution functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testBootRomExecution();
    /**
     * @brief Constructor for SecurityFeaturesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the bootloadersignatureverification functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testBootloaderSignatureVerification();
    /**
     * @brief Constructor for SecurityFeaturesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the kernelsignatureverification functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testKernelSignatureVerification();
    /**
     * @brief Constructor for SecurityFeaturesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the securebootchainoftrust functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSecureBootChainOfTrust();
    /**
     * @brief Constructor for SecurityFeaturesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the hardwarerootoftrust functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testHardwareRootOfTrust();
    /**
     * @brief Constructor for SecurityFeaturesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the securestorage functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSecureStorage();
    /**
     * @brief Constructor for SecurityFeaturesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the cryptographicaccelerators functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCryptographicAccelerators();
    /**
     * @brief Constructor for SecurityFeaturesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the hardwarerandomnumbergenerator functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testHardwareRandomNumberGenerator();
    /**
     * @brief Constructor for SecurityFeaturesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the memoryencryption functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMemoryEncryption();
    /**
     * @brief Constructor for SecurityFeaturesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the debugportsecurity functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDebugPortSecurity();
    /**
     * @brief Constructor for SecurityFeaturesTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the jtagsecurity functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testJtagSecurity();

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

#endif // SECURITYFEATURESTESTING_H
