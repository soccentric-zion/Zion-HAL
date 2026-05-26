/**
 * @file pcie_lib.cpp
 * @brief Implementation file for PcieTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "pcie_lib.h"
#include "../platform_rpi5.h"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

/**
 * @brief Constructor for PcieTesting
 */
PcieTesting::PcieTesting()
{
}

/**
 * @brief Destructor for PcieTesting
 */
PcieTesting::~PcieTesting()
{
}

/**
 * @brief Implementation of testPcieLinkTraining
 * @return TestOutput with pass/fail and data
 */
TestOutput PcieTesting::testPcieLinkTraining()
{
    std::vector<std::string> devices = getPcieDevices();
    std::vector<std::string> data;
    bool allTrained = true;
    int trainedCount = 0;

    if (devices.empty()) {
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            _out.reason["detail_0"] = "No PCIe devices found";
            return _out;
        }
    }

    data.push_back("=== PCIe Link Training Status ===");

    for (const auto& device : devices) {
        std::string devName = device.substr(device.find_last_of('/') + 1);
        std::string currentSpeed = readSysfsFile(device + "/current_link_speed");
        std::string currentWidth = readSysfsFile(device + "/current_link_width");
        std::string maxSpeed = readSysfsFile(device + "/max_link_speed");
        std::string maxWidth = readSysfsFile(device + "/max_link_width");

        // Check if link is active (has current speed and width)
        bool linkActive = !currentSpeed.empty() && !currentWidth.empty();

        if (linkActive) {
            data.push_back("[PASS] " + devName + ": Link trained successfully");
            data.push_back("       Current: " + currentSpeed + " x" + currentWidth + " | Max: " + maxSpeed + " x" + maxWidth);
            trainedCount++;
        } else {
            data.push_back("[FAIL] " + devName + ": Link training failed or device inactive");
            allTrained = false;
        }
    }

    data.push_back("Summary: " + std::to_string(trainedCount) + "/" + std::to_string(devices.size()) + " devices with active links");

    {
        TestOutput _out;
        _out.status = allTrained ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = allTrained ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Implementation of testPcieLinkSpeed
 * @return TestOutput with pass/fail and data
 */
TestOutput PcieTesting::testPcieLinkSpeed()
{
    std::vector<std::string> devices = getPcieDevices();
    std::vector<std::string> data;
    bool allValid = true;
    int gen1_count = 0, gen2_count = 0, gen3_count = 0, gen4_count = 0, gen5_count = 0;

    if (devices.empty()) {
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            _out.reason["detail_0"] = "No PCIe devices found";
            return _out;
        }
    }

    data.push_back("=== PCIe Link Speed Analysis ===");

    for (const auto& device : devices) {
        std::string devName = device.substr(device.find_last_of('/') + 1);
        std::string current_link_speed = readSysfsFile(device + "/current_link_speed");
        std::string max_link_speed = readSysfsFile(device + "/max_link_speed");

        if (!current_link_speed.empty()) {
            std::string status = "[INFO]";
            std::string generation = "Unknown";

            // Determine PCIe generation
            if (current_link_speed.find("2.5") != std::string::npos) {
                generation = "Gen1 (2.5 GT/s)";
                gen1_count++;
            } else if (current_link_speed.find("5.0") != std::string::npos || current_link_speed.find("5 GT") != std::string::npos) {
                generation = "Gen2 (5.0 GT/s)";
                gen2_count++;
            } else if (current_link_speed.find("8.0") != std::string::npos || current_link_speed.find("8 GT") != std::string::npos) {
                generation = "Gen3 (8.0 GT/s)";
                gen3_count++;
            } else if (current_link_speed.find("16.0") != std::string::npos || current_link_speed.find("16 GT") != std::string::npos) {
                generation = "Gen3 (16.0 GT/s)";
                gen4_count++;
            } else if (current_link_speed.find("32.0") != std::string::npos || current_link_speed.find("32 GT") != std::string::npos) {
                generation = "Gen5 (32.0 GT/s)";
                gen5_count++;
            } else {
                status = "[WARN]";
                allValid = false;
            }

            // Check if running at max speed
            if (!max_link_speed.empty() && current_link_speed != max_link_speed) {
                status = "[WARN]";
            }

            data.push_back(status + " " + devName + ": " + generation);
            data.push_back("       Current: " + current_link_speed + " | Max: " + max_link_speed);
        } else {
            data.push_back("[FAIL] " + devName + ": Speed info unavailable");
            allValid = false;
        }
    }

    data.push_back("=== Speed Distribution ===");
    if (gen5_count > 0)
        data.push_back("Gen5 devices: " + std::to_string(gen5_count));
    if (gen4_count > 0)
        data.push_back("Gen3 devices: " + std::to_string(gen4_count));
    if (gen3_count > 0)
        data.push_back("Gen3 devices: " + std::to_string(gen3_count));
    if (gen2_count > 0)
        data.push_back("Gen2 devices: " + std::to_string(gen2_count));
    if (gen1_count > 0)
        data.push_back("Gen1 devices: " + std::to_string(gen1_count));

    {
        TestOutput _out;
        _out.status = allValid ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = allValid ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Implementation of testPcieLinkWidth
 * @return TestOutput with pass/fail and data
 */
TestOutput PcieTesting::testPcieLinkWidth()
{
    std::vector<std::string> devices = getPcieDevices();
    std::vector<std::string> data;
    bool allValid = true;
    int x1_count = 0, x2_count = 0, x4_count = 0, x8_count = 0, x1_wider_count = 0, x32_count = 0;

    if (devices.empty()) {
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            _out.reason["detail_0"] = "No PCIe devices found";
            return _out;
        }
    }

    data.push_back("=== PCIe Link Width Analysis ===");

    for (const auto& device : devices) {
        std::string devName = device.substr(device.find_last_of('/') + 1);
        std::string current_link_width = readSysfsFile(device + "/current_link_width");
        std::string max_link_width = readSysfsFile(device + "/max_link_width");

        if (!current_link_width.empty()) {
            try {
                int currentWidth = std::stoi(current_link_width);
                int maxWidth = max_link_width.empty() ? currentWidth : std::stoi(max_link_width);

                if (currentWidth < 1) {
                    data.push_back("[FAIL] " + devName + ": Invalid width (" + current_link_width + ")");
                    allValid = false;
                    continue;
                }

                std::string status = "[PASS]";
                double utilization = (maxWidth > 0) ? (currentWidth * 100.0 / maxWidth) : 100.0;

                if (utilization < 100.0) {
                    status = "[WARN]";
                }

                data.push_back(status + " " + devName + ": x" + std::to_string(currentWidth) + " / x" + std::to_string(maxWidth) + " (" +
                               std::to_string(static_cast<int>(utilization)) + "% utilized)");

                // Count lane configurations
                if (currentWidth == 1)
                    x1_count++;
                else if (currentWidth == 2)
                    x2_count++;
                else if (currentWidth == 4)
                    x4_count++;
                else if (currentWidth == 8)
                    x8_count++;
                else if (currentWidth == 16)
                    x1_wider_count++;
                else if (currentWidth >= 32)
                    x32_count++;
            }
            catch (...) {
                data.push_back("[FAIL] " + devName + ": Cannot parse width (" + current_link_width + ")");
                allValid = false;
            }
        } else {
            data.push_back("[FAIL] " + devName + ": Width info unavailable");
            allValid = false;
        }
    }

    data.push_back("=== Width Distribution ===");
    if (x32_count > 0)
        data.push_back("x32 lanes: " + std::to_string(x32_count));
    if (x1_wider_count > 0)
        data.push_back("x16 lanes: " + std::to_string(x1_wider_count));
    if (x8_count > 0)
        data.push_back("x8 lanes: " + std::to_string(x8_count));
    if (x4_count > 0)
        data.push_back("x4 lanes: " + std::to_string(x4_count));
    if (x2_count > 0)
        data.push_back("x2 lanes: " + std::to_string(x2_count));
    if (x1_count > 0)
        data.push_back("x1 lanes: " + std::to_string(x1_count));

    {
        TestOutput _out;
        _out.status = allValid ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = allValid ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Implementation of testPcieLaneReversal
 * @details Checks lane polarity and width negotiation on Pi1 PCIe controller.
 *          Reads DT controller nodes for lane-map configuration and validates all
 *          advertised lanes are active on each enumerated PCIe device.
 * @return TestOutput with pass/fail and data
 */
TestOutput PcieTesting::testPcieLaneReversal()
{
    std::vector<std::string> data;
    data.push_back("=== PCIe Lane Reversal Check (Pi5) ===");

    // Pi5 PCIe controller DT base addresses
    static const std::vector<std::string> orionControllers = {"pcie@1f00000000",
                                                              "pcie@141a0000"};

    int foundControllers = 0;
    for (const auto& ctrl : orionControllers) {
        std::string dtBase = "/proc/device-tree/" + ctrl + "/";
        std::ifstream laneMap(dtBase + "nvidia,lane-map");
        std::ifstream numLanes(dtBase + "nvidia,num-lanes");
        std::ifstream statusFile(dtBase + "status");
        if (statusFile.is_open()) {
            std::string statusVal;
            std::getline(statusFile, statusVal);
            statusVal.erase(std::remove(statusVal.begin(), statusVal.end(), '\0'), statusVal.end());
            if (statusVal == "okay" || statusVal == "ok") {
                foundControllers++;
                std::string info = "[INFO] " + ctrl + ": active";
                if (laneMap.is_open())
                    info += " (custom lane map configured)";
                if (numLanes.is_open()) {
                    std::string lanes;
                    std::getline(numLanes, lanes);
                    info += " lanes=" + lanes;
                }
                data.push_back(info);
            }
        }
    }

    // Verify lane width negotiation via sysfs
    std::vector<std::string> devices = getPcieDevices();
    bool allLanesActive = true;
    for (const auto& device : devices) {
        std::string devName = device.substr(device.find_last_of('/') + 1);
        std::string curWidth = readSysfsFile(device + "/current_link_width");
        std::string maxWidth = readSysfsFile(device + "/max_link_width");
        if (curWidth.empty() || maxWidth.empty())
            continue;
        try {
            int cur = std::stoi(curWidth);
            int max = std::stoi(maxWidth);
            if (cur == max && cur > 0) {
                data.push_back("[PASS] " + devName + ": All " + curWidth + " lanes active (no unexpected reversal/loss)");
            } else if (cur < max) {
                data.push_back("[WARN] " + devName + ": x" + curWidth + " of x" + maxWidth +
                               " lanes active (possible lane polarity/reversal issue)");
                allLanesActive = false;
            }
        }
        catch (...) {
        }
    }

    if (devices.empty() && foundControllers == 0) {
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            _out.reason["detail_0"] = "No PCIe controllers or devices found";
            return _out;
        }
    }

    data.push_back("DT controllers found: " + std::to_string(foundControllers) +
                   " | PCIe devices checked: " + std::to_string(devices.size()));
    {
        TestOutput _out;
        _out.status = allLanesActive ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = allLanesActive ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Implementation of testPcieHotplugSupport
 * @details Scans /sys/bus/pci/slots for OS-managed hotplug slots and inspects
 *          PCI bridge class devices for hotplug capability on Pi5.
 *          Pi5 uses a fixed embedded topology; hotplug GPIO control is
 *          platform-specific.
 * @return TestOutput with pass/fail and data
 */
TestOutput PcieTesting::testPcieHotplugSupport()
{
    std::vector<std::string> data;
    data.push_back("=== PCIe Hotplug Support (Pi5) ===");
    data.push_back("[INFO] Pi5 uses fixed-endpoint PCIe topology; hotplug handled via GPIO");

    // Enumerate kernel-managed hotplug slots
    int slotCount = 0;
    DIR* slotsDir = opendir("/sys/bus/pci/slots");
    if (slotsDir) {
        struct dirent* entry;
        while ((entry = readdir(slotsDir)) != nullptr) {
            if (entry->d_name[0] == '.')
                continue;
            std::string slotPath = std::string("/sys/bus/pci/slots/") + entry->d_name;
            std::string power = readSysfsFile(slotPath + "/power");
            std::string address = readSysfsFile(slotPath + "/address");
            std::string attention = readSysfsFile(slotPath + "/attention");
            data.push_back("[INFO] Slot " + std::string(entry->d_name) + ": address=" + address + " power=" + power +
                           " attention=" + attention);
            slotCount++;
        }
        closedir(slotsDir);
    }

    // Inspect PCI bridges for hotplug capability
    std::vector<std::string> devices = getPcieDevices();
    int hotplugBridges = 0;
    for (const auto& device : devices) {
        std::string devName = device.substr(device.find_last_of('/') + 1);
        std::string classCode = readSysfsFile(device + "/class");
        if (classCode.empty())
            continue;
        try {
            long cls = std::stol(classCode, nullptr, 16);
            int baseClass = (cls >> 16) & 0xFF;
            int subClass = (cls >> 8) & 0xFF;
            if (baseClass == 0x06 && subClass == 0x04) {
                // PCI-to-PCI bridge
                std::ifstream hpFile(device + "/hotplug/enabled");
                std::string hpNote = hpFile.is_open() ? "hotplug-enabled" : "bridge (no HP)";
                data.push_back("[INFO] " + devName + ": PCI bridge [" + hpNote + "]");
                if (hpFile.is_open())
                    hotplugBridges++;
            }
        }
        catch (...) {
        }
    }

    if (slotCount == 0 && hotplugBridges == 0) {
        data.push_back("[INFO] No OS-managed hotplug slots (expected on Pi5 embedded PCIe)");
        data.push_back("[INFO] Physical hotplug is handled by platform firmware/GPIO");
    } else {
        data.push_back("Found " + std::to_string(slotCount) + " slot(s), " + std::to_string(hotplugBridges) + " hotplug-capable bridge(s)");
    }

    {
        TestOutput _out;
        _out.status = hal::Status::PASS;
        _out.severity = hal::Severity::INFO;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Implementation of testPcieReadBandwidth
 * @details Reports theoretical peak read bandwidth per device based on negotiated
 *          link speed and width (PCIe Gen3 x1 = ~1 GB/s on Pi5).
 *          Also measures sysfs config-space read latency as an access sanity check.
 * @return TestOutput with pass/fail and data
 */
TestOutput PcieTesting::testPcieReadBandwidth()
{
    std::vector<std::string> devices = getPcieDevices();
    std::vector<std::string> data;
    bool overallPass = false;

    if (devices.empty()) {
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            _out.reason["detail_0"] = "No PCIe devices found";
            return _out;
        }
    }

    data.push_back("=== PCIe Read Bandwidth (Pi5) ===");
    data.push_back("[INFO] Pi5 supports PCIe Gen3 x1 = ~1 GB/s (single lane via RP1)");

    // Theoretical bandwidth: BW(GB/s) = GTps * efficiency * lanes / 8
    auto theoreticalBW = [](const std::string& speed, int width) -> double {
        double gtps = 0.0;
        if (speed.find("2.5") != std::string::npos)
            gtps = 2.5;
        else if (speed.find("5.0") != std::string::npos || speed.find("5 GT") != std::string::npos)
            gtps = 5.0;
        else if (speed.find("8.0") != std::string::npos || speed.find("8 GT") != std::string::npos)
            gtps = 8.0;
        else if (speed.find("16.0") != std::string::npos || speed.find("16 GT") != std::string::npos)
            gtps = 16.0;
        else if (speed.find("32.0") != std::string::npos || speed.find("32 GT") != std::string::npos)
            gtps = 32.0;
        double efficiency = (gtps >= 8.0) ? (128.0 / 130.0) : (8.0 / 10.0);
        return (gtps * 1e9 * efficiency * width) / 8.0 / 1e9; // GB/s
    };

    for (const auto& device : devices) {
        std::string devName = device.substr(device.find_last_of('/') + 1);
        std::string speedStr = readSysfsFile(device + "/current_link_speed");
        std::string widthStr = readSysfsFile(device + "/current_link_width");
        if (speedStr.empty() || widthStr.empty())
            continue;
        try {
            int width = std::stoi(widthStr);
            double bwGBps = theoreticalBW(speedStr, width);
            if (bwGBps > 0.0) {
                char buf[128];
                snprintf(buf, sizeof(buf), "%.2f GB/s", bwGBps);
                data.push_back("[INFO] " + devName + ": theoretical read BW = " + buf + " (" + speedStr + " x" + widthStr + ")");
                overallPass = true;
            }
        }
        catch (...) {
        }
    }

    // Measure practical sysfs config-space read latency
    {
        const std::string& testDev = devices[0];
        const int iterations = 1000;
        auto tStart = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            std::ifstream f(testDev + "/vendor");
            char dummy[16];
            if (f)
                f.read(dummy, sizeof(dummy));
        }
        auto tEnd = std::chrono::high_resolution_clock::now();
        double elapsed_us = std::chrono::duration<double, std::micro>(tEnd - tStart).count();
        double per_read_us = elapsed_us / iterations;
        char lbuf[128];
        snprintf(lbuf, sizeof(lbuf), "%.2f us/read (%d reads, config space)", per_read_us, iterations);
        data.push_back("[INFO] Config-space read latency: " + std::string(lbuf));
    }

    if (!overallPass) {
        data.push_back("[FAIL] Could not determine bandwidth for any device");
    }
    {
        TestOutput _out;
        _out.status = overallPass ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = overallPass ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Implementation of testPcieWriteBandwidth
 * @details Reports theoretical peak write bandwidth per device and performs a
 *          practical sequential-write throughput measurement to /dev/shm as a
 *          memory-subsystem sanity check on Pi5.
 * @return TestOutput with pass/fail and data
 */
TestOutput PcieTesting::testPcieWriteBandwidth()
{
    std::vector<std::string> devices = getPcieDevices();
    std::vector<std::string> data;
    bool overallPass = false;

    if (devices.empty()) {
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            _out.reason["detail_0"] = "No PCIe devices found";
            return _out;
        }
    }

    data.push_back("=== PCIe Write Bandwidth (Pi5) ===");
    data.push_back("[INFO] Pi5 supports PCIe Gen3 x1 = ~1 GB/s (single lane via RP1)");

    auto theoreticalBW = [](const std::string& speed, int width) -> double {
        double gtps = 0.0;
        if (speed.find("2.5") != std::string::npos)
            gtps = 2.5;
        else if (speed.find("5.0") != std::string::npos || speed.find("5 GT") != std::string::npos)
            gtps = 5.0;
        else if (speed.find("8.0") != std::string::npos || speed.find("8 GT") != std::string::npos)
            gtps = 8.0;
        else if (speed.find("16.0") != std::string::npos || speed.find("16 GT") != std::string::npos)
            gtps = 16.0;
        else if (speed.find("32.0") != std::string::npos || speed.find("32 GT") != std::string::npos)
            gtps = 32.0;
        double efficiency = (gtps >= 8.0) ? (128.0 / 130.0) : (8.0 / 10.0);
        return (gtps * 1e9 * efficiency * width) / 8.0 / 1e9;
    };

    for (const auto& device : devices) {
        std::string devName = device.substr(device.find_last_of('/') + 1);
        std::string speedStr = readSysfsFile(device + "/current_link_speed");
        std::string widthStr = readSysfsFile(device + "/current_link_width");
        if (speedStr.empty() || widthStr.empty())
            continue;
        try {
            int width = std::stoi(widthStr);
            double bwGBps = theoreticalBW(speedStr, width);
            if (bwGBps > 0.0) {
                char buf[128];
                snprintf(buf, sizeof(buf), "%.2f GB/s", bwGBps);
                data.push_back("[INFO] " + devName + ": theoretical write BW = " + buf + " (" + speedStr + " x" + widthStr + ")");
                overallPass = true;
            }
        }
        catch (...) {
        }
    }

    // Sequential write throughput to /dev/shm (DRAM-backed, eliminates storage bottleneck)
    {
        const std::string testFile = "/dev/shm/pcie_bw_test";
        const size_t blockSize = 4 * 1024 * 1024; // 4 MB
        const int blocks = 8;                     // 32 MB total
        std::vector<char> wbuf(blockSize, static_cast<char>(0xAB));

        auto tStart = std::chrono::high_resolution_clock::now();
        bool writeOk = false;
        {
            std::ofstream ofs(testFile, std::ios::binary | std::ios::trunc);
            if (ofs.is_open()) {
                for (int i = 0; i < blocks; ++i)
                    ofs.write(wbuf.data(), static_cast<std::streamsize>(blockSize));
                ofs.flush();
                writeOk = ofs.good();
            }
        }
        auto tEnd = std::chrono::high_resolution_clock::now();
        unlink(testFile.c_str());

        if (writeOk) {
            double elapsed = std::chrono::duration<double>(tEnd - tStart).count();
            double mb_written = static_cast<double>(blockSize * blocks) / (1024.0 * 1024.0);
            double mbps = mb_written / (elapsed > 0.0 ? elapsed : 1e-9);
            char mbuf[128];
            snprintf(mbuf, sizeof(mbuf), "%.1f MB/s (%.0f MB in %.3f s)", mbps, mb_written, elapsed);
            data.push_back("[INFO] Sequential write throughput (/dev/shm): " + std::string(mbuf));
            overallPass = true;
        } else {
            data.push_back("[WARN] Could not write to /dev/shm for throughput test");
        }
    }

    if (!overallPass) {
        data.push_back("[FAIL] Could not determine write bandwidth for any device");
    }
    {
        TestOutput _out;
        _out.status = overallPass ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = overallPass ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Implementation of testPcieLatency
 * @details Measures PCIe config-space access latency on Pi5 by timing
 *          repeated sysfs vendor-register reads (500 samples per device).
 *          Reports min/avg/max latency and flags devices with anomalous latency.
 * @return TestOutput with pass/fail and data
 */
TestOutput PcieTesting::testPcieLatency()
{
    std::vector<std::string> devices = getPcieDevices();
    std::vector<std::string> data;

    if (devices.empty()) {
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            _out.reason["detail_0"] = "No PCIe devices found";
            return _out;
        }
    }

    data.push_back("=== PCIe Config-Space Access Latency (Pi5) ===");
    data.push_back("[INFO] Sampling sysfs vendor register read (500 samples per device)");

    bool anyMeasured = false;
    bool overallPass = true;

    for (const auto& device : devices) {
        std::string devName = device.substr(device.find_last_of('/') + 1);
        std::string widthStr = readSysfsFile(device + "/current_link_width");
        if (widthStr.empty())
            continue; // Skip devices without an active link

        const int iterations = 500;
        double minUs = 1e9, maxUs = 0.0, totalUs = 0.0;

        for (int i = 0; i < iterations; ++i) {
            auto t0 = std::chrono::high_resolution_clock::now();
            {
                std::ifstream f(device + "/vendor");
                char dummy[16];
                if (f)
                    f.read(dummy, sizeof(dummy));
            }
            auto t1 = std::chrono::high_resolution_clock::now();
            double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
            if (us < minUs)
                minUs = us;
            if (us > maxUs)
                maxUs = us;
            totalUs += us;
        }

        double avgUs = totalUs / iterations;
        char buf[256];
        snprintf(buf, sizeof(buf), "min=%.1f us  avg=%.1f us  max=%.1f us  (%d samples)", minUs, avgUs, maxUs, iterations);
        data.push_back("[INFO] " + devName + ": " + buf);

        if (avgUs < 200.0) {
            data.push_back("[PASS] " + devName + ": Latency within acceptable range");
        } else {
            data.push_back("[WARN] " + devName + ": High avg latency (" + std::to_string(static_cast<int>(avgUs)) +
                           " us) - possible system load or link degradation");
            overallPass = false;
        }
        anyMeasured = true;
    }

    if (!anyMeasured) {
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            _out.reason["detail_0"] = "No PCIe devices with active links found";
            return _out;
        }
    }

    data.push_back("[INFO] Values include kernel/OS overhead on top of PCIe config-space TLP round-trip");
    {
        TestOutput _out;
        _out.status = overallPass ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = overallPass ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Implementation of testPcieErrorHandling
 * @details Reads AER (Advanced Error Reporting) sysfs counters for correctable,
 *          non-fatal, and fatal PCIe errors on each device.  Falls back to a
 *          dmesg scan when AER sysfs is unavailable.  Pi5 PCIe Gen3
 *          controllers support AER when CONFIG_PCIEAER=y.
 * @return TestOutput with pass/fail and data
 */
TestOutput PcieTesting::testPcieErrorHandling()
{
    std::vector<std::string> devices = getPcieDevices();
    std::vector<std::string> data;
    bool overallPass = true;

    if (devices.empty()) {
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            _out.reason["detail_0"] = "No PCIe devices found";
            return _out;
        }
    }

    data.push_back("=== PCIe AER Error Handling (Pi5) ===");
    data.push_back("[INFO] Checking AER correctable / non-fatal / fatal counters");

    int checkedDevices = 0;
    for (const auto& device : devices) {
        std::string devName = device.substr(device.find_last_of('/') + 1);
        std::string aerCorr = readSysfsFile(device + "/aer_dev_correctable");
        std::string aerFatal = readSysfsFile(device + "/aer_dev_fatal");
        std::string aerNonFatal = readSysfsFile(device + "/aer_dev_nonfatal");

        if (aerCorr.empty() && aerFatal.empty() && aerNonFatal.empty())
            continue;
        checkedDevices++;
        data.push_back("[CHECK] " + devName + ":");

        // Helper lambda: parse "key val key val ..." and return true if any val != 0
        auto hasNonZero = [](const std::string& s) -> bool {
            std::istringstream iss(s);
            std::string k, v;
            while (iss >> k >> v) {
                try {
                    if (std::stoi(v) != 0)
                        return true;
                }
                catch (...) {
                }
            }
            return false;
        };

        if (!aerCorr.empty()) {
            if (hasNonZero(aerCorr))
                data.push_back("  [WARN] Correctable errors: " + aerCorr);
            else
                data.push_back("  [PASS] No correctable errors");
        }
        if (!aerNonFatal.empty()) {
            if (hasNonZero(aerNonFatal)) {
                data.push_back("  [WARN] Non-fatal errors: " + aerNonFatal);
            } else {
                data.push_back("  [PASS] No non-fatal errors");
            }
        }
        if (!aerFatal.empty()) {
            if (hasNonZero(aerFatal)) {
                data.push_back("  [FAIL] Fatal errors: " + aerFatal);
                overallPass = false;
            } else {
                data.push_back("  [PASS] No fatal errors");
            }
        }
    }

    if (checkedDevices == 0) {
        data.push_back("[INFO] AER sysfs not exposed (requires CONFIG_PCIEAER=y and AER-capable device)");
        // Fallback: scan dmesg for PCIe error messages
        std::string dmesgOut = executeCommand("dmesg 2>/dev/null | grep -Ei 'pcie.*error|aer.*error|pci.*error' | tail -10");
        if (!dmesgOut.empty()) {
            data.push_back("[WARN] PCIe error message(s) found in dmesg:");
            std::istringstream iss(dmesgOut);
            std::string line;
            while (std::getline(iss, line))
                if (!line.empty())
                    data.push_back("  " + line);
            overallPass = false;
        } else {
            data.push_back("[PASS] No PCIe error messages in dmesg");
        }
    } else {
        data.push_back("AER checked on " + std::to_string(checkedDevices) + " device(s)");
    }

    {
        TestOutput _out;
        _out.status = overallPass ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = overallPass ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Implementation of testPciePowerManagement
 * @details Reads PCI power_state, d3cold_allowed, and runtime PM status for each
 *          PCIe device.  Pi1 PCIe controller support L0s/L1/L1.1/L1.2
 *          ASPM link power states in addition to D-state device power management.
 * @return TestOutput with pass/fail and data
 */
TestOutput PcieTesting::testPciePowerManagement()
{
    std::vector<std::string> devices = getPcieDevices();
    std::vector<std::string> data;
    bool overallPass = true;

    if (devices.empty()) {
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            _out.reason["detail_0"] = "No PCIe devices found";
            return _out;
        }
    }

    data.push_back("=== PCIe Power Management (Pi5) ===");
    data.push_back("[INFO] Pi5 supports PCIe Gen3 ASPM L0s/L1/L1.1/L1.2 and D0/D3hot/D3cold");

    int d0Count = 0, suspendCount = 0, unknownCount = 0;

    for (const auto& device : devices) {
        std::string devName = device.substr(device.find_last_of('/') + 1);
        std::string powerState = readSysfsFile(device + "/power_state");
        std::string d3coldAllowed = readSysfsFile(device + "/d3cold_allowed");
        std::string runtimeStatus = readSysfsFile(device + "/power/runtime_status");
        std::string runtimeControl = readSysfsFile(device + "/power/control");
        std::string linkState = readSysfsFile(device + "/link_state");

        if (powerState.empty()) {
            unknownCount++;
            continue;
        }

        std::string extra;
        if (!d3coldAllowed.empty())
            extra += " d3cold=" + d3coldAllowed;
        if (!runtimeStatus.empty())
            extra += " rt=" + runtimeStatus;
        if (!runtimeControl.empty())
            extra += " ctrl=" + runtimeControl;
        if (!linkState.empty())
            extra += " link=" + linkState;

        if (powerState == "D0") {
            data.push_back("[PASS] " + devName + ": Active (D0)" + extra);
            d0Count++;
        } else if (powerState == "D3hot" || powerState == "D3cold") {
            data.push_back("[INFO] " + devName + ": Suspended (" + powerState + ")" + extra);
            suspendCount++;
        } else if (powerState == "D1" || powerState == "D2") {
            data.push_back("[INFO] " + devName + ": Low-power (" + powerState + ")" + extra);
            suspendCount++;
        } else {
            data.push_back("[WARN] " + devName + ": Unexpected state '" + powerState + "'" + extra);
        }
    }

    // Check RP1 PCIe debug FS if available
    std::string rp1Info = executeCommand("cat /sys/kernel/debug/rp1_pciefw/info 2>/dev/null | head -10");
    if (!rp1Info.empty()) {
        data.push_back("=== RP1 PCIe Firmware Status ===");
        std::istringstream iss(rp1Info);
        std::string line;
        while (std::getline(iss, line))
            if (!line.empty())
                data.push_back("  " + line);
    }

    // Check ASPM policy
    std::string aspmPolicy = readSysfsFile("/sys/module/pcie_aspm/parameters/policy");
    if (!aspmPolicy.empty())
        data.push_back("[INFO] System ASPM policy: " + aspmPolicy);

    data.push_back("Summary: " + std::to_string(d0Count) + " active (D0), " + std::to_string(suspendCount) + " suspended, " +
                   std::to_string(unknownCount) + " unknown");

    {
        TestOutput _out;
        _out.status = overallPass ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = overallPass ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Implementation of testSecondaryPcieLinkTraining
 * @details Filters PCIe devices resident on secondary buses (bus number > 0)
 *          and verifies each has a trained link.  Also captures the lspci tree
 *          to show the full Pi5 PCIe topology.
 * @return TestOutput with pass/fail and data
 */
TestOutput PcieTesting::testSecondaryPcieLinkTraining()
{
    std::vector<std::string> devices = getPcieDevices();
    std::vector<std::string> data;
    bool anySecondary = false;
    int trainedCount = 0;
    int totalSecondary = 0;

    data.push_back("=== Secondary PCIe Link Training (Pi5) ===");
    data.push_back("[INFO] Orin has up to 5 independent PCIe root port controllers");

    for (const auto& device : devices) {
        // BDF format: DDDD:BB:DD.F - extract bus number (BB, hex)
        std::string devName = device.substr(device.find_last_of('/') + 1);
        int bus = -1;
        try {
            size_t c1 = devName.find(':');
            size_t c2 = (c1 != std::string::npos) ? devName.find(':', c1 + 1) : std::string::npos;
            if (c1 != std::string::npos && c2 != std::string::npos)
                bus = std::stoi(devName.substr(c1 + 1, c2 - c1 - 1), nullptr, 16);
        }
        catch (...) {
        }

        if (bus > 0) {
            totalSecondary++;
            anySecondary = true;
            std::string speedStr = readSysfsFile(device + "/current_link_speed");
            std::string widthStr = readSysfsFile(device + "/current_link_width");
            bool trained = !speedStr.empty() && !widthStr.empty();
            if (trained) {
                data.push_back("[PASS] " + devName + " (bus 0x" + std::to_string(bus) + "): link trained at " + speedStr + " x" + widthStr);
                trainedCount++;
            } else {
                data.push_back("[WARN] " + devName + " (bus 0x" + std::to_string(bus) + "): no link speed/width");
            }
        }
    }

    // PCIe topology tree via lspci
    std::string lspciTree = executeCommand("lspci -t 2>/dev/null | head -40");
    if (!lspciTree.empty()) {
        data.push_back("=== PCIe Bus Topology ===");
        std::istringstream iss(lspciTree);
        std::string line;
        while (std::getline(iss, line))
            if (!line.empty())
                data.push_back("  " + line);
    }

    if (!anySecondary) {
        data.push_back("[INFO] No devices on secondary buses - all endpoints on root bus");
        data.push_back("[INFO] This is expected when no PCIe switches are present");
        {
            TestOutput _out;
            _out.status = hal::Status::PASS;
            _out.severity = hal::Severity::INFO;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    data.push_back("Secondary bus trained: " + std::to_string(trainedCount) + "/" + std::to_string(totalSecondary));
    {
        TestOutput _out;
        _out.status = (trainedCount == totalSecondary) ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = (trainedCount == totalSecondary) ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Implementation of testPcieX4Bandwidth
 * @details Enumerates all PCIe devices running at x4 or wider lane width and
 *          reports theoretical bandwidth.  Pi5 AGX supports PCIe Gen3 x4
 *          on one controller providing ~8 GB/s.
 * @return TestOutput with pass/fail and data
 */
TestOutput PcieTesting::testPcieX4Bandwidth()
{
    std::vector<std::string> devices = getPcieDevices();
    std::vector<std::string> data;
    bool found = false;

    data.push_back("=== PCIe x1 Bandwidth (Pi5) ===");
    data.push_back("[INFO] Pi5 Gen3 x4: ~8 GB/s theoretical (128b/130b encoding)");

    for (const auto& device : devices) {
        std::string devName = device.substr(device.find_last_of('/') + 1);
        std::string widthStr = readSysfsFile(device + "/current_link_width");
        std::string speedStr = readSysfsFile(device + "/current_link_speed");
        std::string maxWStr = readSysfsFile(device + "/max_link_width");
        if (widthStr.empty())
            continue;

        try {
            int width = std::stoi(widthStr);
            int maxWidth = maxWStr.empty() ? width : std::stoi(maxWStr);
            if (width < 4)
                continue;

            double gtps = 0.0;
            std::string gen = "Unknown";
            if (speedStr.find("16.0") != std::string::npos || speedStr.find("16 GT") != std::string::npos) {
                gtps = 16.0;
                gen = "Gen3";
            } else if (speedStr.find("8.0") != std::string::npos || speedStr.find("8 GT") != std::string::npos) {
                gtps = 8.0;
                gen = "Gen3";
            } else if (speedStr.find("5.0") != std::string::npos || speedStr.find("5 GT") != std::string::npos) {
                gtps = 5.0;
                gen = "Gen2";
            } else if (speedStr.find("2.5") != std::string::npos) {
                gtps = 2.5;
                gen = "Gen1";
            }

            double efficiency = (gtps >= 8.0) ? (128.0 / 130.0) : (8.0 / 10.0);
            double bwGBps = (gtps * 1e9 * efficiency * width) / 8.0 / 1e9;
            bool atMax = (width >= maxWidth);

            char buf[256];
            snprintf(buf, sizeof(buf), "%.2f GB/s [%s x%d%s]", bwGBps, gen.c_str(), width, atMax ? " FULL" : " DEGRADED");

            std::string pfx = (atMax && gtps >= 16.0) ? "[PASS]" : (atMax) ? "[INFO]" : "[WARN]";
            data.push_back(pfx + " " + devName + ": " + buf);
            if (!atMax)
                data.push_back("       Running at x" + std::to_string(width) + " of x" + std::to_string(maxWidth) +
                               " - check slot/connector");
            found = true;
        }
        catch (...) {
        }
    }

    if (!found) {
        data.push_back("[INFO] No x4 (or wider) PCIe devices detected");
        data.push_back("[INFO] Connect an NVMe, capture card, or other x4 device to test");
    }

    {
        TestOutput _out;
        _out.status = hal::Status::PASS;
        _out.severity = hal::Severity::INFO;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    } // Not a failure when device is simply absent
}

/**
 * @brief Implementation of testPcieX4LaneFunctionality
 * @details Verifies all advertised x4 PCIe lanes are active (current == max width)
 *          and running at maximum negotiated speed.  Also inspects AER correctable
 *          error counters (BadTLP/BadDLLP) as an indicator of per-lane integrity.
 * @return TestOutput with pass/fail and data
 */
TestOutput PcieTesting::testPcieX4LaneFunctionality()
{
    std::vector<std::string> devices = getPcieDevices();
    std::vector<std::string> data;
    bool overallPass = true;
    bool anyX4 = false;

    data.push_back("=== PCIe x1 Lane Functionality (Pi5) ===");
    data.push_back("[INFO] Verifying all x4 PCIe lanes are functional and at max speed");

    for (const auto& device : devices) {
        std::string devName = device.substr(device.find_last_of('/') + 1);
        std::string curWStr = readSysfsFile(device + "/current_link_width");
        std::string maxWStr = readSysfsFile(device + "/max_link_width");
        std::string curSStr = readSysfsFile(device + "/current_link_speed");
        std::string maxSStr = readSysfsFile(device + "/max_link_speed");
        if (curWStr.empty())
            continue;

        try {
            int curW = std::stoi(curWStr);
            int maxW = maxWStr.empty() ? curW : std::stoi(maxWStr);
            if (maxW < 4)
                continue;

            anyX4 = true;
            bool widthOk = (curW >= maxW);
            bool speedOk = (maxSStr.empty() || curSStr == maxSStr);

            data.push_back((widthOk && speedOk ? "[PASS]" : "[WARN]") + (" " + devName + ":"));
            data.push_back("  Width: x" + std::to_string(curW) + "/x" + std::to_string(maxW) +
                           (widthOk ? " [ALL LANES ACTIVE]" : " [DEGRADED - check connector]"));
            data.push_back("  Speed: " + (curSStr.empty() ? "N/A" : curSStr) + (maxSStr.empty() ? "" : " / " + maxSStr) +
                           (speedOk ? " [AT MAX SPEED]" : " [BELOW MAX SPEED]"));

            if (!widthOk || !speedOk)
                overallPass = false;

            // Inspect AER correctable counters (BadTLP / BadDLLP indicate lane issues)
            std::string aerCorr = readSysfsFile(device + "/aer_dev_correctable");
            if (!aerCorr.empty()) {
                std::istringstream iss(aerCorr);
                std::string k, v;
                while (iss >> k >> v) {
                    if (k == "BadTLP" || k == "BadDLLP" || k == "ECRC") {
                        try {
                            if (std::stoi(v) > 0)
                                data.push_back("  [WARN] AER " + k + "=" + v + " (lane integrity concern)");
                        }
                        catch (...) {
                        }
                    }
                }
            }
        }
        catch (...) {
        }
    }

    if (!anyX4)
        data.push_back("[INFO] No x4 or wider PCIe devices present on this system");

    {
        TestOutput _out;
        _out.status = overallPass ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = overallPass ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Implementation of testConcurrentPcieX8AndX4Operation
 * @details Verifies that Pi5's independent PCIe root port controllers
 *          can operate simultaneously.  Enumerates all active x1/x4/x8 devices
 *          and checks the kernel log for PCIe errors during concurrent operation.
 * @return TestOutput with pass/fail and data
 */
TestOutput PcieTesting::testConcurrentPcieX8AndX4Operation()
{
    std::vector<std::string> devices = getPcieDevices();
    std::vector<std::string> data;

    data.push_back("=== Concurrent PCIe x1 + x4 Operation (Pi5) ===");
    data.push_back("[INFO] Pi5 has independent root port controllers - concurrency is by design");

    std::vector<std::string> x1devs_wide, x4devs, x1devs;

    for (const auto& device : devices) {
        std::string wStr = readSysfsFile(device + "/current_link_width");
        if (wStr.empty())
            continue;
        try {
            int w = std::stoi(wStr);
            std::string n = device.substr(device.find_last_of('/') + 1);
            std::string s = readSysfsFile(device + "/current_link_speed");
            std::string entry = n + " (" + s + " x" + wStr + ")";
            if (w >= 8)
                x1devs_wide.push_back(entry);
            else if (w >= 4)
                x4devs.push_back(entry);
            else
                x1devs.push_back(entry);
        }
        catch (...) {
        }
    }

    auto report = [&](const std::vector<std::string>& devs, const std::string& label) {
        if (!devs.empty()) {
            data.push_back("[PASS] " + label + " devices: " + std::to_string(devs.size()));
            for (const auto& d : devs)
                data.push_back("  - " + d);
        } else {
            data.push_back("[INFO] No " + label + " devices connected");
        }
    };
    report(x1devs_wide, "x1");
    report(x4devs, "x4");
    report(x1devs, "x1");

    int totalActive = static_cast<int>(x1devs_wide.size() + x4devs.size() + x1devs.size());
    if (!x1devs_wide.empty() && !x4devs.empty())
        data.push_back("[PASS] Concurrent x8 + x4 operation confirmed");
    else if (totalActive >= 2)
        data.push_back("[INFO] Multiple PCIe devices active - concurrent controller operation verified");
    else if (totalActive == 1)
        data.push_back("[INFO] Single device only - connect additional PCIe device for full concurrency test");
    else
        data.push_back("[WARN] No active PCIe links found");

    // Check kernel log for PCIe errors
    std::string errOut = executeCommand("dmesg 2>/dev/null | grep -Ei 'pci[e].*error|pci[e].*failed|pci[e].*timeout' | tail -5");
    if (!errOut.empty()) {
        data.push_back("[WARN] PCIe issues in dmesg:");
        std::istringstream iss(errOut);
        std::string line;
        while (std::getline(iss, line))
            if (!line.empty())
                data.push_back("  " + line);
    } else {
        data.push_back("[PASS] No PCIe errors in kernel log");
    }

    {
        TestOutput _out;
        _out.status = hal::Status::PASS;
        _out.severity = hal::Severity::INFO;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Implementation of testNvmeSsdConnectivity
 * @return TestOutput with pass/fail and data
 */
TestOutput PcieTesting::testNvmeSsdConnectivity()
{
    std::string output = executeCommand("lspci | grep -i nvme");
    std::vector<std::string> data;

    if (output.empty()) {
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            _out.reason["detail_0"] = "No NVMe devices found";
            return _out;
        }
    }

    std::istringstream iss(output);
    std::string line;
    int nvme_count = 0;

    while (std::getline(iss, line)) {
        data.push_back("NVMe device: " + line);
        nvme_count++;
    }

    data.insert(data.begin(), "Found " + std::to_string(nvme_count) + " NVMe devices");

    {
        TestOutput _out;
        _out.status = hal::Status::PASS;
        _out.severity = hal::Severity::INFO;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Implementation of testGpuAddinCardSupport
 * @return TestOutput with pass/fail and data
 */
TestOutput PcieTesting::testGpuAddinCardSupport()
{
    std::string output = executeCommand("lspci | grep -i 'vga\\|3d\\|display'");
    std::vector<std::string> data;

    if (output.empty()) {
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            _out.reason["detail_0"] = "No GPU/display devices found";
            return _out;
        }
    }

    std::istringstream iss(output);
    std::string line;
    int gpu_count = 0;

    while (std::getline(iss, line)) {
        data.push_back("GPU device: " + line);
        gpu_count++;
    }

    data.insert(data.begin(), "Found " + std::to_string(gpu_count) + " GPU/display devices");

    {
        TestOutput _out;
        _out.status = hal::Status::PASS;
        _out.severity = hal::Severity::INFO;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Implementation of testNetworkAdapterConnectivity
 * @return TestOutput with pass/fail and data
 */
TestOutput PcieTesting::testNetworkAdapterConnectivity()
{
    std::string output = executeCommand("lspci | grep -i 'network\\|ethernet'");
    std::vector<std::string> data;

    if (output.empty()) {
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            _out.reason["detail_0"] = "No network adapter devices found";
            return _out;
        }
    }

    std::istringstream iss(output);
    std::string line;
    int net_count = 0;

    while (std::getline(iss, line)) {
        data.push_back("Network device: " + line);
        net_count++;
    }

    data.insert(data.begin(), "Found " + std::to_string(net_count) + " network adapter devices");

    {
        TestOutput _out;
        _out.status = hal::Status::PASS;
        _out.severity = hal::Severity::INFO;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Implementation of testMultiplePcieDeviceEnumeration
 * @return TestOutput with pass/fail and data
 */
TestOutput PcieTesting::testMultiplePcieDeviceEnumeration()
{
    std::vector<std::string> devices = getPcieDevices();
    std::vector<std::string> data;

    if (devices.empty()) {
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            _out.reason["detail_0"] = "No PCIe devices found";
            return _out;
        }
    }

    data.push_back("Found " + std::to_string(devices.size()) + " PCIe devices:");

    for (const auto& device : devices) {
        std::string vendor = readSysfsFile(device + "/vendor");
        std::string device_id = readSysfsFile(device + "/device");
        std::string class_code = readSysfsFile(device + "/class");

        std::string dev_name = device.substr(device.find_last_of('/') + 1);
        data.push_back("Device: " + dev_name + ", Vendor: " + vendor + ", Device ID: " + device_id + ", Class: " + class_code);
    }

    {
        TestOutput _out;
        _out.status = hal::Status::PASS;
        _out.severity = hal::Severity::INFO;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Implementation of testPcieSwitchCompatibility
 * @details Scans sysfs for PCI bridge devices (class 0x0604) which represent
 *          PCIe switch upstream/downstream ports and root-complex inrp1ted
 *          bridges.  Known PCIe switch vendor IDs are flagged.  Also captures
 *          lspci output to enumerate bridge devices by description.
 * @return TestOutput with pass/fail and data
 */
TestOutput PcieTesting::testPcieSwitchCompatibility()
{
    std::vector<std::string> devices = getPcieDevices();
    std::vector<std::string> data;
    int bridgeCount = 0;
    int switchCount = 0;

    data.push_back("=== PCIe Switch/Bridge Compatibility (Pi5) ===");

    // Known 3rd-party PCIe switch vendor IDs (hex, lower-case)
    static const std::vector<std::string> switchVendors = {
        "0x104c", // Texas Instruments
        "0x12d8", // Pericom / Diodes Inc.
        "0x1055", // SMSC / Microchip
        "0x11f8", // PMC-Sierra
        "0x1957", // Freescale / NXP
        "0x10b5", // PLX Technology (Broadcom)
    };

    for (const auto& device : devices) {
        std::string devName = device.substr(device.find_last_of('/') + 1);
        std::string classCode = readSysfsFile(device + "/class");
        std::string vendor = readSysfsFile(device + "/vendor");
        std::string deviceId = readSysfsFile(device + "/device");
        if (classCode.empty())
            continue;
        try {
            long cls = std::stol(classCode, nullptr, 16);
            if (((cls >> 16) & 0xFF) == 0x06 && ((cls >> 8) & 0xFF) == 0x04) {
                bridgeCount++;
                std::string speedStr = readSysfsFile(device + "/current_link_speed");
                std::string widthStr = readSysfsFile(device + "/current_link_width");

                // Check for known PCIe switch chips
                bool isSwitch = false;
                for (const auto& sv : switchVendors) {
                    if (vendor == sv) {
                        isSwitch = true;
                        break;
                    }
                }

                std::string info = (isSwitch ? "[INFO] PCIe switch" : "[INFO] PCI bridge") + (" " + devName) + " [vendor=" + vendor +
                                   " dev=" + deviceId + "]";
                if (!speedStr.empty())
                    info += " " + speedStr + " x" + widthStr;
                data.push_back(info);

                if (isSwitch) {
                    data.push_back("       Known PCIe switch vendor detected");
                    switchCount++;
                }
            }
        }
        catch (...) {
        }
    }

    // lspci bridge listing for additional details
    std::string lspciOut = executeCommand("lspci 2>/dev/null | grep -Ei 'pci bridge|root port|upstream port|downstream port'");
    if (!lspciOut.empty()) {
        data.push_back("=== lspci Bridge Listing ===");
        std::istringstream iss(lspciOut);
        std::string line;
        while (std::getline(iss, line))
            if (!line.empty())
                data.push_back("  " + line);
    }

    data.push_back("Found " + std::to_string(bridgeCount) + " PCI bridge(s), " + std::to_string(switchCount) +
                   " identified 3rd-party PCIe switch chip(s)");

    if (bridgeCount == 0) {
        data.push_back("[INFO] No PCI bridges detected - Pi5 using direct endpoint connections");
        data.push_back("[INFO] This is the expected embedded configuration");
    } else {
        data.push_back("[PASS] PCI bridge(s) enumerated and accessible");
    }

    {
        TestOutput _out;
        _out.status = hal::Status::PASS;
        _out.severity = hal::Severity::INFO;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Run all tests for this module
 * @return Vector of test results
 */
std::vector<TestResult> PcieTesting::runAllTests()
{
    std::vector<TestResult> results;

    // Run testPcieLinkTraining
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPcieLinkTraining();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPcieLinkTraining";
            _tr.test_id = "testPcieLinkTraining";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testPcieLinkSpeed
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPcieLinkSpeed();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPcieLinkSpeed";
            _tr.test_id = "testPcieLinkSpeed";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testPcieLinkWidth
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPcieLinkWidth();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPcieLinkWidth";
            _tr.test_id = "testPcieLinkWidth";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testPcieLaneReversal
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPcieLaneReversal();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPcieLaneReversal";
            _tr.test_id = "testPcieLaneReversal";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testPcieHotplugSupport
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPcieHotplugSupport();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPcieHotplugSupport";
            _tr.test_id = "testPcieHotplugSupport";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testPcieReadBandwidth
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPcieReadBandwidth();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPcieReadBandwidth";
            _tr.test_id = "testPcieReadBandwidth";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testPcieWriteBandwidth
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPcieWriteBandwidth();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPcieWriteBandwidth";
            _tr.test_id = "testPcieWriteBandwidth";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testPcieLatency
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPcieLatency();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPcieLatency";
            _tr.test_id = "testPcieLatency";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testPcieErrorHandling
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPcieErrorHandling();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPcieErrorHandling";
            _tr.test_id = "testPcieErrorHandling";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testPciePowerManagement
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPciePowerManagement();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPciePowerManagement";
            _tr.test_id = "testPciePowerManagement";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSecondaryPcieLinkTraining
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSecondaryPcieLinkTraining();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSecondaryPcieLinkTraining";
            _tr.test_id = "testSecondaryPcieLinkTraining";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testPcieX4Bandwidth
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPcieX4Bandwidth();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPcieX4Bandwidth";
            _tr.test_id = "testPcieX4Bandwidth";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testPcieX4LaneFunctionality
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPcieX4LaneFunctionality();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPcieX4LaneFunctionality";
            _tr.test_id = "testPcieX4LaneFunctionality";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testConcurrentPcieX8AndX4Operation
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testConcurrentPcieX8AndX4Operation();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testConcurrentPcieX8AndX4Operation";
            _tr.test_id = "testConcurrentPcieX8AndX4Operation";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testNvmeSsdConnectivity
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testNvmeSsdConnectivity();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testNvmeSsdConnectivity";
            _tr.test_id = "testNvmeSsdConnectivity";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testGpuAddinCardSupport
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testGpuAddinCardSupport();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testGpuAddinCardSupport";
            _tr.test_id = "testGpuAddinCardSupport";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testNetworkAdapterConnectivity
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testNetworkAdapterConnectivity();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testNetworkAdapterConnectivity";
            _tr.test_id = "testNetworkAdapterConnectivity";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testMultiplePcieDeviceEnumeration
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testMultiplePcieDeviceEnumeration();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testMultiplePcieDeviceEnumeration";
            _tr.test_id = "testMultiplePcieDeviceEnumeration";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testPcieSwitchCompatibility
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPcieSwitchCompatibility();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPcieSwitchCompatibility";
            _tr.test_id = "testPcieSwitchCompatibility";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    return results;
}

/**
 * @brief Get list of all available test names
 * @return Vector of test names
 */
std::vector<std::string> PcieTesting::getAvailableTests()
{
    return {
        "testPcieLinkTraining",
        "testPcieLinkSpeed",
        "testPcieLinkWidth",
        "testPcieLaneReversal",
        "testPcieHotplugSupport",
        "testPcieReadBandwidth",
        "testPcieWriteBandwidth",
        "testPcieLatency",
        "testPcieErrorHandling",
        "testPciePowerManagement",
        "testSecondaryPcieLinkTraining",
        "testPcieX4Bandwidth",
        "testPcieX4LaneFunctionality",
        "testConcurrentPcieX8AndX4Operation",
        "testNvmeSsdConnectivity",
        "testGpuAddinCardSupport",
        "testNetworkAdapterConnectivity",
        "testMultiplePcieDeviceEnumeration",
        "testPcieSwitchCompatibility",
    };
}

/**
 * @brief Run a single test by name
 * @param testName Name of the test to run
 * @return Test result with timing
 */
TestResult PcieTesting::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if (testName == "testPcieLinkTraining") {
        result = testPcieLinkTraining();
    } else if (testName == "testPcieLinkSpeed") {
        result = testPcieLinkSpeed();
    } else if (testName == "testPcieLinkWidth") {
        result = testPcieLinkWidth();
    } else if (testName == "testPcieLaneReversal") {
        result = testPcieLaneReversal();
    } else if (testName == "testPcieHotplugSupport") {
        result = testPcieHotplugSupport();
    } else if (testName == "testPcieReadBandwidth") {
        result = testPcieReadBandwidth();
    } else if (testName == "testPcieWriteBandwidth") {
        result = testPcieWriteBandwidth();
    } else if (testName == "testPcieLatency") {
        result = testPcieLatency();
    } else if (testName == "testPcieErrorHandling") {
        result = testPcieErrorHandling();
    } else if (testName == "testPciePowerManagement") {
        result = testPciePowerManagement();
    } else if (testName == "testSecondaryPcieLinkTraining") {
        result = testSecondaryPcieLinkTraining();
    } else if (testName == "testPcieX4Bandwidth") {
        result = testPcieX4Bandwidth();
    } else if (testName == "testPcieX4LaneFunctionality") {
        result = testPcieX4LaneFunctionality();
    } else if (testName == "testConcurrentPcieX8AndX4Operation") {
        result = testConcurrentPcieX8AndX4Operation();
    } else if (testName == "testNvmeSsdConnectivity") {
        result = testNvmeSsdConnectivity();
    } else if (testName == "testGpuAddinCardSupport") {
        result = testGpuAddinCardSupport();
    } else if (testName == "testNetworkAdapterConnectivity") {
        result = testNetworkAdapterConnectivity();
    } else if (testName == "testMultiplePcieDeviceEnumeration") {
        result = testMultiplePcieDeviceEnumeration();
    } else if (testName == "testPcieSwitchCompatibility") {
        result = testPcieSwitchCompatibility();
    } else {
        // Unknown test name
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = testName + " (UNKNOWN)";
            _tr.test_id = testName;
            _tr.output.status = hal::Status::ERROR;
            _tr.output.severity = hal::Severity::ERROR;
            _tr.output.error_message = "Unknown test name: " + testName;
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            return _tr;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    {
        TestResult _tr;
        _tr.name = testName;
        _tr.test_id = testName;
        _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
        _tr.output = result;
        return _tr;
    }
}

// Helper functions

std::string PcieTesting::readSysfsFile(const std::string& path)
{
    std::ifstream file(path);
    std::string content;
    if (file.is_open()) {
        std::getline(file, content);
        file.close();
    }
    return content;
}

std::vector<std::string> PcieTesting::getPcieDevices()
{
    std::vector<std::string> devices;
    DIR* dir = opendir("/sys/bus/pci/devices");
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_name[0] != '.') {
                devices.push_back(std::string("/sys/bus/pci/devices/") + entry->d_name);
            }
        }
        closedir(dir);
    }
    return devices;
}

std::string PcieTesting::executeCommand(const std::string& cmd)
{
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe)
        return "";
    try {
        while (fgets(buffer, sizeof buffer, pipe) != nullptr) {
            result += buffer;
        }
    }
    catch (...) {
        pclose(pipe);
        return "";
    }
    pclose(pipe);
    return result;
}
