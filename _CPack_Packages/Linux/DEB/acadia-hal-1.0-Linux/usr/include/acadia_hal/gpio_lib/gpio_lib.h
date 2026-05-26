/**
#include "lib_base.h"
 * @file gpio_lib.h
 * @brief Header file for GpioTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef GPIOTESTING_H
#define GPIOTESTING_H
#include "lib_base.h"

#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

/**
 * @enum GpioDirection
 * @brief GPIO pin direction
 */
enum class GpioDirection
{
    INPUT,
    OUTPUT
};

/**
 * @enum GpioPullMode
 * @brief GPIO pull-up/pull-down mode
 */
enum class GpioPullMode
{
    NONE,
    PULL_UP,
    PULL_DOWN
};

/**
 * @struct GpioPin
 * @brief Structure representing a GPIO pin
 */
struct GpioPin
{
    int pinNumber;
    GpioDirection direction;
    bool value;
    GpioPullMode pullMode;
    bool interruptEnabled;
    bool externallyDriven; // Flag to indicate if pin is externally driven
};

/**
 * @class class GpioTesting : public TestBase
 * @brief Class for testing gpiotesting
 * @details This class provides methods to test various gpiotesting functionalities.
 */
class GpioTesting : public TestBase
{
  public: /**
           * @brief Constructor for GpioTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function GpioTesting
           * @return TestOutput with pass/fail and data
           */

    GpioTesting();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~GpioTesting();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpioout1 functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testGpioout1();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpioout2 functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioout2();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpioout3 functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioout3();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpioout4 functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioout4();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpiooutputvoltagelevels functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioOutputVoltageLevels();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpiooutputcurrentdrivecapability functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioOutputCurrentDriveCapability();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpiooutputswitchingspeed functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioOutputSwitchingSpeed();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpiopwmoutput functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioPwmOutput();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpiopulluppulldownresistors functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioPulluppulldownResistors();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpioin1 functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioin1();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpioin2 functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioin2();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpioin3 functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioin3();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpioin4 functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioin4();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpioinputvoltagethreshold functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioInputVoltageThreshold();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpioinputdebouncing functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioInputDebouncing();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpiointerruptgeneration functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioInterruptGeneration();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpiotimercounterfunctionality functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioTimerCounterFunctionality();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpioschmitttrigger functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioSchmittTrigger();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpiodirectionchange functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioDirectionChange();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpioconcurrentaccess functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioConcurrentAccess();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpiointerruptlatency functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioInterruptLatency();
    /**
     * @brief Constructor for GpioTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the gpioreservedpins functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpioReservedPins();

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

    // Expanded: run all tests and return results
    /**
     * @brief Tests the runalltests functionality
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Function runAllTests
     * @return TestOutput with pass/fail and data
     */
  protected:
    std::vector<TestResult> runAllTests() override;

  private:
    // GPIO simulation
    std::map<int, GpioPin> gpioPins_;
    std::mutex gpioMutex_;
    const int NUM_TEST_PINS = 8; // Number of pins to test

    // Helper methods
    bool initializeGpioPins();
    bool setPinDirection(int pin, GpioDirection direction);
    bool setPinValue(int pin, bool value);
    bool getPinValue(int pin, bool& value);
    bool setPinPullMode(int pin, GpioPullMode mode);
    double measureSwitchingSpeed(int pin);
    bool testPwmGeneration(int pin, double frequency, double dutyCycle);
    bool testInterruptGeneration(int pin);
    double measureInterruptLatency(int pin);
    bool testConcurrentAccess(int pin1, int pin2);
};

#endif // GPIOTESTING_H
