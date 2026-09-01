/**
 * @file can_bus_lib.h
 * @brief Header file for CanBusTesting class — PS CAN 2.0B controllers
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC
 *
 * Zynq UltraScale+ MPSoC carries two CAN 2.0B controllers (ff060000.can /
 * ff070000.can) exposed by Linux as SocketCAN interfaces (can0, can1).
 * Validation is read-only unless explicit opt-in (HAL_ALLOW_CAN_TX=1).
 */
#ifndef ZION_CAN_BUS_TESTING_H
#define ZION_CAN_BUS_TESTING_H

#include "lib_base.h"

#include <string>
#include <vector>

/**
 * @class CanBusTesting
 * @brief Class for testing CAN bus (SocketCAN) functionality
 */
class CanBusTesting : public TestBase
{
  public:
    CanBusTesting();
    ~CanBusTesting();

    TestOutput testCanInterfacePresence();
    TestOutput testCanLinkState();
    TestOutput testCanSocketOpen();
    TestOutput testCanErrorCounters();
    TestOutput testCanLoopbackSendReceive();

    std::vector<std::string> getAvailableTests() override;

  protected:
    std::vector<TestResult> runAllTests() override;
    TestResult runSingleTest(const std::string& testName) override;
};

#endif // ZION_CAN_BUS_TESTING_H