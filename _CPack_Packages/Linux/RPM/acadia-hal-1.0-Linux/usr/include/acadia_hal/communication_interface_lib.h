/**
#include "lib_base.h"
 * @file communication_interface_lib.h
 * @brief Header file for CommunicationInterfaceTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef COMMUNICATIONINTERFACETESTING_H
#define COMMUNICATIONINTERFACETESTING_H
#include "lib_base.h"

/**
 * @class class CommunicationInterfaceTesting : public TestBase
 * @brief Class for testing communicationinterfacetesting
 * @details This class provides methods to test various communicationinterfacetesting functionalities.
 */
class CommunicationInterfaceTesting : public TestBase
{
  public: /**
           * @brief Constructor for CommunicationInterfaceTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function CommunicationInterfaceTesting
           * @return TestOutput with pass/fail and data
           */

    CommunicationInterfaceTesting();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~CommunicationInterfaceTesting();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the alluartportsfunctionality functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testAllUartPortsFunctionality();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the variousbaudrates functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testVariousBaudRates();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the hardwareflowcontrol functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testHardwareFlowControl();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the paritychecking functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testParityChecking();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the stopbitconfiguration functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testStopBitConfiguration();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the uartdmaoperation functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testUartDmaOperation();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the uartinterrupthandling functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testUartInterruptHandling();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the consoledebuguart functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testConsoledebugUart();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the allspicontrollers functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAllSpiControllers();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the spimastermode functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSpiMasterMode();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the spislavemode functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSpiSlaveMode();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the variousspiclockspeeds functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testVariousSpiClockSpeeds();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the spimodes functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSpiModes();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the chipselectfunctionality functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testChipSelectFunctionality();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the spidmatransfers functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSpiDmaTransfers();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the multiplespidevices functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMultipleSpiDevices();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the alli2cbuses functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testAllI2cBuses();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the i2cmastermode functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testI2cMasterMode();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the i2cslavemode functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testI2cSlaveMode();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the standardmode functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testStandardMode();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the fastmode functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testFastMode();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the fastmodeplus functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testFastModePlus();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the highspeedmode functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testHighspeedMode();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the i2cclockstretching functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testI2cClockStretching();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the multimasterarbitration functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMultimasterArbitration();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the i2crepeatedstart functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testI2cRepeatedStart();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the can20a functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCan20a();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the can20b functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCan20b();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the canfd functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCanFd();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the variouscanbitrates functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testVariousCanBitrates();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the canerrordetection functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCanErrorDetection();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the canbusoffrecovery functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCanBusoffRecovery();
    /**
     * @brief Constructor for CommunicationInterfaceTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the canmessagefiltering functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCanMessageFiltering();

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

#endif // COMMUNICATIONINTERFACETESTING_H
