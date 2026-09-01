/*
 * @file fpga_fabric_lib.h
 * @brief Header file for FpgaFabricTesting class — FPGA (PL) fabric management and validation
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC
 *
 * Covers the PL half of the Zynq UltraScale+ MPSoC on the Kria KV260:
 *  - Linux fpga_manager sysfs (bitstream state, flags, region name)
 *  - PL clock tree (pl0_ref..pl3_ref via zynqmp-firmware clock sysfs)
 *  - PL IP discovery over the AMBA/AXI bus (/sys/bus/amba/devices)
 *  - Kria acceleration stack (xmutil, dfx-mgr, xclbin/DTBO inventory)
 *  - live AXI register access through the PS-PL UIO backend (optional)
 */
#ifndef ZION_FPGA_FABRIC_TESTING_H
#define ZION_FPGA_FABRIC_TESTING_H

#include "lib_base.h"

#include <string>
#include <vector>

/**
 * @class FpgaFabricTesting
 * @brief Tests the FPGA (PL) fabric path: fpga_manager, PL clocks, AXI IP
 *        enumeration, Kria xmutil/dfx-mgr stack and the PS-PL register backend.
 */
class FpgaFabricTesting : public TestBase
{
  public:
    FpgaFabricTesting();
    ~FpgaFabricTesting();

    TestOutput testFpgaManagerPresence();
    TestOutput testFpgaManagerStateRead();
    TestOutput testPlAxiDeviceEnumeration();
    TestOutput testPlClockControl();
    TestOutput testKriaAcceleratorStack();
    TestOutput testPlInterruptLines();
    TestOutput testPlGpioControllerPresence();
    TestOutput testPsPlRegisterBackend();

    std::vector<std::string> getAvailableTests() override;

  protected:
    std::vector<TestResult> runAllTests() override;
    TestResult runSingleTest(const std::string& testName) override;
};

#endif // ZION_FPGA_FABRIC_TESTING_H