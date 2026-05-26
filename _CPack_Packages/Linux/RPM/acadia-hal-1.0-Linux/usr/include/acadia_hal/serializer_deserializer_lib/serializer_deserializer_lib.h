/**
#include "lib_base.h"
 * @file serializer_deserializer_lib.h
 * @brief Header file for SerializerDeserializerTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef SERIALIZERDESERIALIZERTESTING_H
#define SERIALIZERDESERIALIZERTESTING_H
#include "lib_base.h"

/**
 * @class class SerializerDeserializerTesting : public TestBase
 * @brief Class for testing serializerdeserializertesting
 * @details This class provides methods to test various serializerdeserializertesting functionalities.
 */
class SerializerDeserializerTesting : public TestBase
{
  public: /**
           * @brief Constructor for SerializerDeserializerTesting
           * @return TestOutput with pass/fail and data
           */
          /**
           * @brief Function SerializerDeserializerTesting
           * @return TestOutput with pass/fail and data
           */

    SerializerDeserializerTesting();
    /**
     * @brief Constructor for SerializerDeserializerTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Destructor for class

     */
    ~SerializerDeserializerTesting();
    /**
     * @brief Constructor for SerializerDeserializerTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the serializationfunctionality functionality
     * @return TestOutput with pass/fail and data
     */

    TestOutput testSerializationFunctionality();
    /**
     * @brief Constructor for SerializerDeserializerTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the dataintegrity functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDataIntegrity();
    /**
     * @brief Constructor for SerializerDeserializerTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the linkspeed functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testLinkSpeed();
    /**
     * @brief Constructor for SerializerDeserializerTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the errordetection functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testErrorDetection();
    /**
     * @brief Constructor for SerializerDeserializerTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the deserializationfunctionality functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testDeserializationFunctionality();
    /**
     * @brief Constructor for SerializerDeserializerTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the clockrecovery functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testClockRecovery();
    /**
     * @brief Constructor for SerializerDeserializerTesting
     * @return TestOutput with pass/fail and data
     */
    /**
     * @brief Tests the multistreamsupport functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMultistreamSupport();

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

#endif // SERIALIZERDESERIALIZERTESTING_H
