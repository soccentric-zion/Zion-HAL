/**
#include "lib_base.h"
 * @file mcu_interface_lib.h
 * @brief Header file for McuInterfaceTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef MCUINTERFACETESTING_H
#define MCUINTERFACETESTING_H
#include "lib_base.h"

/**
 * @class class McuInterfaceTesting : public TestBase
 * @brief Class for testing mcuinterfacetesting
 * @details This class provides methods to test various mcuinterfacetesting functionalities.
 */
class McuInterfaceTesting : public TestBase
{
  public: /**
           * @brief Constructor for McuInterfaceTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function McuInterfaceTesting
           * @return TestOutput with pass/fail and data
           */

    McuInterfaceTesting();
    /**
     * @brief Constructor for McuInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~McuInterfaceTesting();
    /**
     * @brief Constructor for McuInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the mcucommunicationinterface functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testMcuCommunicationInterface();
    /**
     * @brief Constructor for McuInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the mcufirmwareoperation functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMcuFirmwareOperation();
    /**
     * @brief Constructor for McuInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the mcuwatchdog functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMcuWatchdog();
    /**
     * @brief Constructor for McuInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the mcugpiointerface functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMcuGpioInterface();
    /**
     * @brief Constructor for McuInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the mcuspiinterface functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMcuSpiInterface();
    /**
     * @brief Constructor for McuInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the mcuuartinterface functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMcuUartInterface();
    /**
     * @brief Constructor for McuInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the mcudmsinterface functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMcuDmsInterface();
    /**
     * @brief Constructor for McuInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the mcutoorindatatransfer functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMcuToOrinDataTransfer();
    /**
     * @brief Constructor for McuInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the secondarymcuredundancy functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSecondaryMcuRedundancy();
    /**
     * @brief Constructor for McuInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the mcufailovermechanism functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMcuFailoverMechanism();
    /**
     * @brief Constructor for McuInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the mcusynchronization functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMcuSynchronization();

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

#endif // MCUINTERFACETESTING_H
