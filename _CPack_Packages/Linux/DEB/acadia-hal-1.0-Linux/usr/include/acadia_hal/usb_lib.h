/**
#include "lib_base.h"
 * @file usb_lib.h
 * @brief Header file for UsbTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef USBTESTING_H
#define USBTESTING_H
#include "lib_base.h"

/**
 * @class class UsbTesting : public TestBase
 * @brief Class for testing usbtesting
 * @details This class provides methods to test various usbtesting functionalities.
 */
class UsbTesting : public TestBase
{
  public: /**
           * @brief Constructor for UsbTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function UsbTesting
           * @return TestOutput with pass/fail and data
           */

    UsbTesting();
    /**
     * @brief Constructor for UsbTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~UsbTesting();
    /**
     * @brief Constructor for UsbTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the usbhostmode functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testUsbHostMode();
    /**
     * @brief Constructor for UsbTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the usbdevicemode functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testUsbDeviceMode();
    /**
     * @brief Constructor for UsbTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the usb32gen2speed functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testUsb32Gen2Speed();
    /**
     * @brief Constructor for UsbTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the usb31gen1backwardcompatibility functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testUsb31Gen1BackwardCompatibility();
    /**
     * @brief Constructor for UsbTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the usb20backwardcompatibility functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testUsb20BackwardCompatibility();
    /**
     * @brief Constructor for UsbTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the usbpowerdelivery functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testUsbPowerDelivery();
    /**
     * @brief Constructor for UsbTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the multipleusbdeviceenumeration functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMultipleUsbDeviceEnumeration();
    /**
     * @brief Constructor for UsbTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the usbhotplugdetection functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testUsbHotplugDetection();
    /**
     * @brief Constructor for UsbTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the usbsuspendresume functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testUsbSuspendresume();
    /**
     * @brief Constructor for UsbTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the usbmassstoragedevices functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testUsbMassStorageDevices();
    /**
     * @brief Constructor for UsbTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the usbcameras functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testUsbCameras();
    /**
     * @brief Constructor for UsbTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the usbnetworkadapters functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testUsbNetworkAdapters();
    /**
     * @brief Constructor for UsbTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the usbhubs functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testUsbHubs();
    /**
     * @brief Constructor for UsbTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the usbaudiodevices functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testUsbAudioDevices();
    /**
     * @brief Constructor for UsbTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the usbhiddevices functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testUsbHidDevices();

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

#endif // USBTESTING_H
