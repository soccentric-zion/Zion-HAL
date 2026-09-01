#ifndef ZION_PLATFORM_BOARD_DETECT_H
#define ZION_PLATFORM_BOARD_DETECT_H

/**
 * @file board_detect.h
 * @brief Runtime board/platform identification for the Zion HAL
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC
 *
 * Identifies the physical board from the device-tree model/compatible strings,
 * cross-checked with /proc/cpuinfo, so every HAL module can report which
 * platform it is actually running on (KV260 starter kit vs. custom Zion
 * Zynq UltraScale+ hardware vs. an unknown host).
 *
 * classifyBoard() is a pure function (no filesystem access) so it is fully
 * unit-testable; detectBoard() is the one-shot runtime wrapper.
 */

#include <string>
#include <vector>

namespace hal {
namespace platform {

/// Boards the HAL knows how to identify.
enum class Board
{
    Unknown,          ///< Unidentified device (desktop host, CI container, ...)
    ZionCustom,       ///< Zion custom Zynq UltraScale+ MPSoC carrier
    KriaKv260,        ///< AMD/Xilinx Kria KV260 Vision AI Starter Kit
    KriaKr260,        ///< AMD/Xilinx Kria KR260 Robotics Starter Kit (sibling)
    KriaK26SOM,       ///< Bare K26 SOM
    ZynqMpGeneric,    ///< Zynq UltraScale+ MPSoC, non-Kria
    Zynq7000,         ///< Zynq-7000 family
    RaspberryPi,      ///< Mis-provisioned target (Pi image) — reported, not supported
};

/// Identification snapshot.
struct BoardInfo
{
    Board board{Board::Unknown};
    std::string model;       ///< device-tree /model (trimmed)
    std::string compatible;  ///< device-tree /compatible, entries joined by space
    std::string soc;         ///< SoC description, e.g. "AMD XCK26 (...)"
    bool hasApuA53{false};   ///< APU quad Cortex-A53 detected
    bool hasRpuR5{false};    ///< RPU dual Cortex-R5F (always true on ZynqMP; confirmed by remoteproc)
    bool hasPl{false};       ///< programmable logic present
    bool hasVcu{false};      ///  H.264/H.265 VCU (EV-class silicon)
    bool hasMali400{false};  ///< Mali-400 MP2 GPU
    bool detectedFromFilesystem{false}; ///< true when live /sys data was used
};

/// Pure classifier — no filesystem access. `compatible` may hold several
/// NUL/space separated entries; matching is case-insensitive substring based.
BoardInfo classifyBoard(const std::string& model,
                        const std::string& compatible,
                        const std::string& cpuinfo);

/// Human readable board name ("Kria KV260 Vision AI Starter Kit", ...).
const char* boardName(Board board);

// ---------------------------------------------------------------------------
// Runtime filesystem helpers (thin wrappers — unit tests use the pure paths)
// ---------------------------------------------------------------------------

/// Read a whole file into `out`; returns false if unavailable.
bool readFile(const std::string& path, std::string& out);

/// Read the first line of `path` (trailing \r\n stripped); returns "" if absent.
std::string readFirstLine(const std::string& path);

/// True when `path` exists (regular file, directory, device node, ...).
bool pathExists(const std::string& path);

/// List entries of a directory (sorted); empty on error.
std::vector<std::string> listDirectory(const std::string& path);

/// Run `cmd` via popen(3) and capture stdout (trailing whitespace trimmed).
std::string runCommandCapture(const std::string& cmd);

/// Enumerate /dev/video* nodes under `dev_dir` and report their V4L2 driver
/// names (ioctl). Entries are "<node> <driver>"; e.g. "/dev/video0 al5e".
/// Used to locate VCU (al5e/al5d), CSI and other V4L2 peripherals.
std::vector<std::string> enumerateV4l2DriverNames(const std::string& dev_dir);

/// One-shot runtime detection: reads model + compatible + cpuinfo.
BoardInfo detectBoard();

/// Convenience: true when the detected board is a Zynq UltraScale+ MPSoC
/// (KV260, KR260, K26, Zion custom or generic ZynqMP).
bool isZynqUltraScalePlus();

} // namespace platform
} // namespace hal

#endif // ZION_PLATFORM_BOARD_DETECT_H