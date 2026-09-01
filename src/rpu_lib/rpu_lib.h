/**
 * @file rpu_lib.h
 * @brief Header file for RpuTesting class — RPU (dual Cortex-R5F) management
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC
 *
 * Validates the Zynq UltraScale+ MPSoC RPU domain on the KV260/K26:
 *  - remoteproc sysfs (boot state, firmware, coredump) of the R5F cores
 *  - RPU operating mode (lockstep vs split) from the zynqmp-firmware/DTS
 *  - OpenAMP rpmsg transport presence (/dev/rpmsg_ctrl0, /sys/bus/rpmsg)
 *  - reserved-memory carve-outs for R5 firmware/DDR regions
 *
 * Lifecycle mutation (start/stop of the R5) is strictly opt-in via the
 * HAL_ALLOW_RPU_CONTROL=1 environment variable: unattended start/stop could
 * kill a live safety application on the board.
 */
#ifndef ZION_RPU_TESTING_H
#define ZION_RPU_TESTING_H

#include "lib_base.h"

#include <map>
#include <string>
#include <vector>

/**
 * @class RpuTesting
 * @brief Class for testing RPU functionality on the Zion/KV260 platform
 */
class RpuTesting : public TestBase
{
  public:
    RpuTesting();
    ~RpuTesting();

    TestOutput testRpuRemoteprocPresence();
    TestOutput testRpuCoreState();
    TestOutput testRpuFirmwareAvailability();
    TestOutput testRpuOperatingMode();
    TestOutput testRpmsgIpcPresence();
    TestOutput testRpuReservedMemory();
    TestOutput testRpuStartStop();

    /// Snapshot of every remoteproc instance discovered under /sys/class/remoteproc.
    /// Maps instance dir -> (firmware, state).
    static std::vector<std::string> listRemoteprocNames(const std::string& rproc_root);

    std::vector<std::string> getAvailableTests() override;

  protected:
    std::vector<TestResult> runAllTests() override;
    TestResult runSingleTest(const std::string& testName) override;
};

#endif // ZION_RPU_TESTING_H