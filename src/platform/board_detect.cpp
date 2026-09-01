/**
 * @file board_detect.cpp
 * @brief Implementation file for runtime board/platform identification — Zion HAL
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "board_detect.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/videodev2.h>

namespace hal {
namespace platform {

// ---------------------------------------------------------------------------
// Local helpers
// ---------------------------------------------------------------------------
namespace {

std::string toLower(const std::string& s)
{
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool containsCI(const std::string& haystack, const std::string& needle)
{
    return toLower(haystack).find(toLower(needle)) != std::string::npos;
}

} // namespace

// ---------------------------------------------------------------------------
// boardName() — stable human-readable identifiers
// ---------------------------------------------------------------------------
const char* boardName(Board board)
{
    switch (board) {
        case Board::ZionCustom:    return "Zion custom Zynq UltraScale+ MPSoC carrier";
        case Board::KriaKv260:     return "AMD Kria KV260 Vision AI Starter Kit";
        case Board::KriaKr260:     return "AMD Kria KR260 Robotics Starter Kit";
        case Board::KriaK26SOM:    return "AMD Kria K26 SOM";
        case Board::ZynqMpGeneric: return "Zynq UltraScale+ MPSoC (generic)";
        case Board::Zynq7000:      return "Zynq-7000 SoC";
        case Board::RaspberryPi:   return "Raspberry Pi (unsupported target)";
        case Board::Unknown:       break;
    }
    return "Unknown board";
}

// ---------------------------------------------------------------------------
// classifyBoard() — pure, unit-testable
// ---------------------------------------------------------------------------
BoardInfo classifyBoard(const std::string& model,
                        const std::string& compatible,
                        const std::string& cpuinfo)
{
    BoardInfo info;
    info.model = model;
    info.compatible = compatible;

    info.hasApuA53 = containsCI(cpuinfo, "aarch64") || containsCI(cpuinfo, "AArch64");

    // Ordered, most specific first. Compatible strings come from the Kria
    // / ZynqMP product device trees.
    if (containsCI(model, "kv260") || containsCI(compatible, "kv260")) {
        info.board = Board::KriaKv260;
        info.hasVcu = true;         // XCK26 is EV-class: VCU present
        info.hasMali400 = true;     // Mali-400 MP2
        info.hasRpuR5 = true;
        info.hasPl = true;
    } else if (containsCI(model, "kr260") || containsCI(compatible, "kr260")) {
        info.board = Board::KriaKr260;
        info.hasVcu = true;
        info.hasMali400 = true;
        info.hasRpuR5 = true;
        info.hasPl = true;
    } else if (containsCI(model, "k26") || containsCI(compatible, "smk-k26")) {
        info.board = Board::KriaK26SOM;
        info.hasVcu = true;
        info.hasMali400 = true;
        info.hasRpuR5 = true;
        info.hasPl = true;
    } else if (containsCI(compatible, "zedboard") || containsCI(compatible, "xlnx,zynq-7000") ||
               containsCI(cpuinfo, "armv7l")) {
        info.board = Board::Zynq7000;
        info.hasPl = true; // Zynq-7000 PL
        info.hasApuA53 = false;
    } else if (containsCI(compatible, "xlnx,zynqmp") || containsCI(cpuinfo, "aarch64") ||
               containsCI(cpuinfo, "AArch64")) {
        // Generic Zynq UltraScale+ MPSoC (Zion custom carrier falls here unless
        // its DT carries a recognizable compatible).
        if (containsCI(model, "zion") || containsCI(compatible, "zion")) {
            info.board = Board::ZionCustom;
        } else {
            info.board = Board::ZynqMpGeneric;
        }
        info.hasApuA53 = true;
        info.hasRpuR5 = true;
        info.hasPl = true;
        // EV-class parts carry a VCU; EG parts do not. Compatible strings do
        // not distinguish EG/EV reliably, so VCU presence is a runtime probe.
        info.hasVcu = false;
        info.hasMali400 = true;
    } else if (containsCI(cpuinfo, "raspberrypi") || containsCI(model, "Raspberry Pi")) {
        info.board = Board::RaspberryPi;
        info.hasApuA53 = true;
        info.hasPl = false;
        info.hasRpuR5 = false;
        info.hasMali400 = false;
        info.hasVcu = false;
    }

    // SoC string
    if (info.board == Board::KriaKv260 || info.board == Board::KriaKr260 ||
        info.board == Board::KriaK26SOM) {
        info.soc = "AMD XCK26 (Zynq UltraScale+ MPSoC, Kria K26 SOM)";
    } else if (info.board == Board::ZionCustom || info.board == Board::ZynqMpGeneric) {
        info.soc = "Xilinx Zynq UltraScale+ MPSoC";
    } else if (info.board == Board::Zynq7000) {
        info.soc = "Xilinx Zynq-7000 SoC";
    } else if (info.board == Board::RaspberryPi) {
        info.soc = "Broadcom BCM2712 (Raspberry Pi 5)";
    } else {
        info.soc = "unknown";
    }

    return info;
}

// ---------------------------------------------------------------------------
// Filesystem helpers
// ---------------------------------------------------------------------------
bool readFile(const std::string& path, std::string& out)
{
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    char buf[4096];
    size_t n;
    out.clear();
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) out.append(buf, n);
    fclose(f);
    return true;
}

std::string readFirstLine(const std::string& path)
{
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return "";
    char buf[512];
    std::string line;
    if (fgets(buf, sizeof(buf), f) == nullptr) {
        fclose(f);
        return "";
    }
    line = buf;
    fclose(f);
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r' || line.back() == '\0')) {
        line.pop_back();
    }
    return line;
}

bool pathExists(const std::string& path)
{
    struct stat st{};
    return stat(path.c_str(), &st) == 0;
}

std::vector<std::string> listDirectory(const std::string& path)
{
    std::vector<std::string> entries;
    DIR* d = opendir(path.c_str());
    if (!d) return entries;
    while (struct dirent* e = readdir(d)) {
        const std::string name(e->d_name);
        if (name == "." || name == "..") continue;
        entries.push_back(name);
    }
    closedir(d);
    std::sort(entries.begin(), entries.end());
    return entries;
}

std::string runCommandCapture(const std::string& cmd)
{
    std::string out;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return out;
    char buf[512];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), pipe)) > 0) out.append(buf, n);
    pclose(pipe);
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
    return out;
}

// ---------------------------------------------------------------------------
// V4L2 driver enumeration — identifies al5e/al5d (VCU), zynqmp CSI, DP-sub...
// ---------------------------------------------------------------------------
std::vector<std::string> enumerateV4l2DriverNames(const std::string& dev_dir)
{
    std::vector<std::string> result;
    for (const auto& name : listDirectory(dev_dir)) {
        if (name.rfind("video", 0) != 0) continue;
        const std::string node = dev_dir + "/" + name;
        int fd = open(node.c_str(), O_RDWR);
        if (fd < 0) {
            fd = open(node.c_str(), O_RDONLY);
        }
        if (fd < 0) {
            result.push_back(node + " (unopenable)");
            continue;
        }
        v4l2_capability caps{};
        if (ioctl(fd, VIDIOC_QUERYCAP, &caps) == 0) {
            std::string driver(reinterpret_cast<char*>(caps.driver), 32);
            driver = driver.c_str(); // stop at NUL
            result.push_back(node + " " + driver);
        } else {
            result.push_back(node + " (ioctl failed)");
        }
        close(fd);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Runtime detection
// ---------------------------------------------------------------------------
BoardInfo detectBoard()
{
    std::string model = readFirstLine("/sys/firmware/devicetree/base/model");
    std::string compat;
    readFile("/sys/firmware/devicetree/base/compatible", compat);
    // /compatible is NUL-separated; normalize to spaces for the classifier.
    std::replace(compat.begin(), compat.end(), '\0', ' ');

    std::string cpuinfo;
    readFile("/proc/cpuinfo", cpuinfo);

    BoardInfo info = classifyBoard(model, compat, cpuinfo);
    info.detectedFromFilesystem = pathExists("/sys/firmware/devicetree/base/model");
    return info;
}

bool isZynqUltraScalePlus()
{
    const Board b = detectBoard().board;
    return b == Board::ZionCustom || b == Board::KriaKv260 || b == Board::KriaKr260 ||
           b == Board::KriaK26SOM || b == Board::ZynqMpGeneric;
}

} // namespace platform
} // namespace hal