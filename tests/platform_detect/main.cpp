/**
 * @file main.cpp
 * @brief Unit + runtime tests for the hal_platform board-detection library
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC
 *
 * Pure-classifier tests run everywhere (no hardware needed); the runtime probe
 * section reports the board this binary is actually executing on.
 */

#include "platform/board_detect.h"

#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void expect(bool cond, const std::string& msg)
{
    if (!cond) {
        std::cerr << "[FAIL] " << msg << "\n";
        ++g_failures;
        return;
    }
    std::cout << "[PASS] " << msg << "\n";
}

using hal::platform::Board;
using hal::platform::BoardInfo;
using hal::platform::classifyBoard;


} // namespace

int main()
{
    // ------------------------------------------------------------------
    // KV260: canonical Kria starter-kit device tree
    // ------------------------------------------------------------------
    {
        const BoardInfo info = classifyBoard(
            "Xilinx Kria KV260 Vision AI Starter Kit",
            "xlnx,zynqmp-smk-k26-revA xlnx,zynqmp-k26-revA xlnx,zynqmp",
            "AArch64 Processor rev 0 (aarch64)");
        expect(info.board == Board::KriaKv260, "KV260 DT model classified as KriaKv260");
        expect(info.hasApuA53, "KV260 APU (Cortex-A53) detected");
        expect(info.hasRpuR5, "KV260 RPU (Cortex-R5F) detected");
        expect(info.hasPl, "KV260 PL detected");
        expect(info.hasVcu, "KV260 VCU flagged (EV-class silicon)");
        expect(info.hasMali400, "KV260 Mali-400 MP2 flagged");
    }

    // ------------------------------------------------------------------
    // KR260 sibling
    // ------------------------------------------------------------------
    {
        const BoardInfo info = classifyBoard(
            "AMD Kria KR260 Robotics Starter Kit", "xlnx,zynqmp-smk-k26-revA", "aarch64");
        expect(info.board == Board::KriaKr260, "KR260 detected in model string");
    }

    // ------------------------------------------------------------------
    // Zion custom carrier
    // ------------------------------------------------------------------
    {
        const BoardInfo info = classifyBoard(
            "Zion custom ZynqMP carrier revB",
            "xlnx,zynqmp",
            "AArch64 Processor rev 0 (aarch64)");
        expect(info.board == Board::ZionCustom, "compatible 'xlnx,zynqmp' + zion model -> ZionCustom");
    }

    // ------------------------------------------------------------------
    // Unrelated devices must not be misdetected as ZynqMP
    // ------------------------------------------------------------------
    {
        const BoardInfo info = classifyBoard(
            "Raspberry Pi 5 Model B Rev 1.0",
            "raspberrypi,5-model-b brcm,bcm2712",
            "Raspberry Pi Compatible");
        expect(info.board == Board::RaspberryPi || info.board == Board::Unknown,
               "Raspberry Pi model not misidentified as ZynqMP");
    }
    {
        const BoardInfo info = classifyBoard(
            "Generic x86_64 host", "", "Intel(R) Xeon(R) CPU");
        expect(info.board == Board::Unknown, "x86 host classified Unknown");
    }

    // ------------------------------------------------------------------
    // Runtime probe (informational on this machine)
    // ------------------------------------------------------------------
    {
        const BoardInfo info = hal::platform::detectBoard();
        std::cout << "\nRuntime detection:\n"
                  << "  model      : " << info.model << "\n"
                  << "  compatible : " << info.compatible << "\n"
                  << "  board      : " << hal::platform::boardName(info.board) << "\n"
                  << "  SoC        : " << info.soc << "\n"
                  << "  APU A53    : " << (info.hasApuA53 ? "yes" : "no") << "\n"
                  << "  RPU R5     : " << (info.hasRpuR5 ? "yes" : "no") << "\n"
                  << "  PL         : " << (info.hasPl ? "yes" : "no") << "\n"
                  << "  VCU        : " << (info.hasVcu ? "yes" : "no") << "\n"
                  << "  Mali-400   : " << (info.hasMali400 ? "yes" : "no") << "\n";
        expect(!info.model.empty() || !info.detectedFromFilesystem,
               "runtime detection returns a non-empty model (or empty on DT-less host)");
    }

    if (g_failures == 0) {
        std::cout << "\nsystem_probe_self_identification: ALL PASS\n";
        return 0;
    }
    std::cerr << g_failures << " check(s) FAILED\n";
    return 1;
}