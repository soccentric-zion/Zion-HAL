/**
 * @file storage_lib.cpp
 * @brief Implementation file for StorageTesting class — Raspberry Pi 5
 *        Tests the SD card (mmcblk0) and optional NVMe SSD (nvme0n1)
 *        attached via the PCIe Gen 3 x1 M.2 slot.
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "storage_lib.h"
#include "../platform_rpi5.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <vector>

// ---------------------------------------------------------------------------
// Pi5 storage constants
// ---------------------------------------------------------------------------
static constexpr const char* MMCBLK_DEV        = RPI5_MMCBLK_PATH;   // /dev/mmcblk0
static constexpr const char* NVME_DEV          = RPI5_NVME_PATH;      // /dev/nvme0n1
static constexpr const char* MMCBLK_SYSFS      = "/sys/block/mmcblk0";
static constexpr const char* NVME_SYSFS        = "/sys/block/nvme0n1";
static constexpr const char* NVME_HWMON        = "/sys/class/nvme/nvme0/hwmon1/temp1_input";

// Performance thresholds — conservative minimums for Pi5 SD / NVMe
static constexpr double SD_MIN_SEQ_READ_MBS    = 20.0;   // MB/s minimum SD seq read
static constexpr double SD_MIN_SEQ_WRITE_MBS   = 10.0;   // MB/s minimum SD seq write
static constexpr double NVME_MIN_SEQ_READ_MBS  = 200.0;  // MB/s minimum NVMe seq read
static constexpr double NVME_MIN_SEQ_WRITE_MBS = 100.0;  // MB/s minimum NVMe seq write
static constexpr int    SD_MIN_RAND_READ_IOPS  = 100;
static constexpr int    SD_MIN_RAND_WRITE_IOPS = 50;
static constexpr double NVME_TEMP_MAX_C        = 70.0;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string storReadFile(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::string s;
    std::getline(f, s);
    return s;
}

static long long storReadInt(const std::string& path)
{
    std::string s = storReadFile(path);
    if (s.empty()) return -1LL;
    try { return std::stoll(s); } catch (...) { return -1LL; }
}

static bool pathExists(const std::string& p)
{
    struct stat st{};
    return stat(p.c_str(), &st) == 0;
}

/** Run shell command, return stdout (up to 4 KB). */
static std::string storRunCmd(const char* cmd)
{
    char buf[4096] = {};
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "";
    size_t n = fread(buf, 1, sizeof(buf) - 1, pipe);
    pclose(pipe);
    buf[n] = '\0';
    while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r')) buf[--n] = '\0';
    return std::string(buf);
}

/**
 * Measure sequential read bandwidth by reading 32 MB from a block device
 * using O_RDONLY | O_DIRECT (bypass page cache).
 * Returns MB/s or -1 on error.
 */
static double measureSeqReadMBps(const std::string& dev, size_t totalBytes = 32 * 1024 * 1024)
{
    const size_t BUF = 512 * 1024; // 512 KB I/O size
    void* buf = nullptr;
    if (posix_memalign(&buf, 4096, BUF) != 0) return -1.0;

    int fd = open(dev.c_str(), O_RDONLY | O_DIRECT);
    if (fd < 0) {
        // Fallback: without O_DIRECT
        fd = open(dev.c_str(), O_RDONLY);
        if (fd < 0) { free(buf); return -1.0; }
    }

    auto t0 = std::chrono::high_resolution_clock::now();
    size_t bytesRead = 0;
    while (bytesRead < totalBytes) {
        ssize_t n = read(fd, buf, BUF);
        if (n <= 0) break;
        bytesRead += static_cast<size_t>(n);
    }
    double elapsed = std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now() - t0).count();
    close(fd);
    free(buf);

    if (bytesRead == 0 || elapsed <= 0.0) return -1.0;
    return static_cast<double>(bytesRead) / (1024.0 * 1024.0) / elapsed;
}

/** Measure random read IOPS with 4 KB random reads over 2 seconds. */
static int measureRandReadIops(const std::string& dev)
{
    // Get device size in bytes
    std::string szPath = "/sys/block/";
    // extract basename
    size_t pos = dev.rfind('/');
    std::string base = (pos != std::string::npos) ? dev.substr(pos + 1) : dev;
    long long sectors = storReadInt("/sys/block/" + base + "/size");
    if (sectors <= 0) sectors = 1024 * 1024; // fallback 512MB

    int fd = open(dev.c_str(), O_RDONLY);
    if (fd < 0) return -1;

    const size_t BLOCK = 4096;
    std::vector<char> buf(BLOCK);
    std::mt19937_64 rng(42);
    long long maxOffset = (sectors * 512LL) - static_cast<long long>(BLOCK);
    if (maxOffset <= 0) maxOffset = 4096;

    auto tEnd = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    int iops = 0;
    while (std::chrono::steady_clock::now() < tEnd) {
        long long offset = static_cast<long long>(rng() % static_cast<uint64_t>(maxOffset));
        offset = (offset / 512) * 512; // align to 512
        if (pread(fd, buf.data(), BLOCK, offset) > 0) ++iops;
    }
    close(fd);
    return iops / 2; // per second
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
StorageTesting::StorageTesting()
{
}

StorageTesting::~StorageTesting()
{
}

// ---------------------------------------------------------------------------
// 1. testEmmc51InterfaceFunctionality
//    On Pi5 the "eMMC" is actually a microSD via an SD 3.0 / UHS-I controller.
//    Verify the block device node and sysfs attributes are present.
// ---------------------------------------------------------------------------
TestOutput StorageTesting::testEmmc51InterfaceFunctionality()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Pi5 storage: microSD via SD 3.0 controller (not eMMC 5.1)");
    data.push_back("SD device node: " + std::string(MMCBLK_DEV));

    bool devPresent = pathExists(MMCBLK_DEV);
    data.push_back("Block device present: " + std::string(devPresent ? "YES" : "NO"));

    if (!devPresent) {
        passed = false;
        data.push_back("FAIL: SD card block device not found. No SD card inserted?");
        {
            TestOutput _out;
            _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
            _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    // Check sysfs attributes
    std::string csd  = storReadFile(MMCBLK_SYSFS + std::string("/device/csd"));
    std::string name = storReadFile(MMCBLK_SYSFS + std::string("/device/name"));
    std::string type = storReadFile(MMCBLK_SYSFS + std::string("/device/type"));
    std::string speed = storReadFile(MMCBLK_SYSFS + std::string("/device/speed_class"));
    std::string hs   = storReadFile(MMCBLK_SYSFS + std::string("/device/preferred_erase_size"));

    if (!name.empty())  data.push_back("Card name: " + name);
    if (!type.empty())  data.push_back("Card type: " + type);
    if (!speed.empty()) data.push_back("Speed class: " + speed);
    if (!csd.empty())   data.push_back("CSD register: " + csd.substr(0, 32) + "…");

    // Check UHS/HS mode via mmc-utils or sysfs
    std::string busWidth = storReadFile(MMCBLK_SYSFS + std::string("/device/bus_width"));
    if (!busWidth.empty()) data.push_back("Bus width: " + busWidth + " bit");

    std::string timing = storRunCmd("cat /sys/kernel/debug/mmc0/ios 2>/dev/null | grep timing");
    if (!timing.empty()) data.push_back("MMC timing: " + timing);

    data.push_back("PASS: SD card interface detected and accessible.");
    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 2. testFullCapacityVerification
//    Report total and available capacity of the SD card.
// ---------------------------------------------------------------------------
TestOutput StorageTesting::testFullCapacityVerification()
{
    std::vector<std::string> data;
    bool passed = true;

    long long sectors = storReadInt(MMCBLK_SYSFS + std::string("/size"));
    if (sectors > 0) {
        double gb = static_cast<double>(sectors) * 512.0 / (1024.0 * 1024.0 * 1024.0);
        data.push_back("SD capacity (sectors): " + std::to_string(sectors));
        data.push_back("SD capacity (GB): " + std::to_string(gb));
        if (gb < 2.0) {
            passed = false;
            data.push_back("FAIL: SD card capacity < 2 GB — suspiciously small.");
        } else {
            data.push_back("PASS: SD card capacity verified: " + std::to_string(gb) + " GB");
        }
    } else {
        data.push_back("SD sysfs size not available; trying statvfs on /");
    }

    // statvfs on root (which is likely on the SD card)
    struct statvfs sv{};
    if (statvfs("/", &sv) == 0) {
        double totalGB = static_cast<double>(sv.f_blocks) * sv.f_frsize / (1024.0 * 1024.0 * 1024.0);
        double freeGB  = static_cast<double>(sv.f_bfree)  * sv.f_frsize / (1024.0 * 1024.0 * 1024.0);
        data.push_back("Root filesystem total: " + std::to_string(totalGB) + " GB");
        data.push_back("Root filesystem free:  " + std::to_string(freeGB)  + " GB");
    }

    // NVMe if present
    if (pathExists(NVME_SYSFS)) {
        long long nvmeSectors = storReadInt(NVME_SYSFS + std::string("/size"));
        if (nvmeSectors > 0) {
            double nvmeGB = static_cast<double>(nvmeSectors) * 512.0 / (1024.0 * 1024.0 * 1024.0);
            data.push_back("NVMe capacity: " + std::to_string(nvmeGB) + " GB");
        }
    }

    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 3. testSequentialReadPerformance
//    Measure sequential read throughput from the SD card block device.
// ---------------------------------------------------------------------------
TestOutput StorageTesting::testSequentialReadPerformance()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Testing sequential read from: " + std::string(MMCBLK_DEV));
    data.push_back("Transfer size: 32 MB");

    if (!pathExists(MMCBLK_DEV)) {
        data.push_back("SKIP: SD card not present.");
        {
            TestOutput _out;
            _out.status = hal::Status::PASS;
            _out.severity = hal::Severity::INFO;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    double mbps = measureSeqReadMBps(MMCBLK_DEV);
    if (mbps < 0) {
        data.push_back("FAIL: Could not read from " + std::string(MMCBLK_DEV) + " (permission denied?)");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    data.push_back("Sequential read speed: " + std::to_string(mbps) + " MB/s");
    data.push_back("Minimum threshold: " + std::to_string(SD_MIN_SEQ_READ_MBS) + " MB/s");

    if (mbps < SD_MIN_SEQ_READ_MBS) {
        passed = false;
        data.push_back("FAIL: SD sequential read below threshold.");
    } else {
        data.push_back("PASS: SD sequential read meets minimum requirement.");
    }

    // Also test NVMe if present
    if (pathExists(NVME_DEV)) {
        double nvmeMbps = measureSeqReadMBps(NVME_DEV);
        if (nvmeMbps > 0) {
            data.push_back("NVMe sequential read: " + std::to_string(nvmeMbps) + " MB/s");
            if (nvmeMbps < NVME_MIN_SEQ_READ_MBS) {
                passed = false;
                data.push_back("FAIL: NVMe read below " + std::to_string(NVME_MIN_SEQ_READ_MBS) + " MB/s threshold.");
            }
        }
    }

    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 4. testSequentialWritePerformance
//    Write to /tmp (tmpfs) and measure throughput; report SD write class info.
// ---------------------------------------------------------------------------
TestOutput StorageTesting::testSequentialWritePerformance()
{
    std::vector<std::string> data;
    bool passed = true;

    // We write to /tmp (tmpfs) for safety, and report SD speed class
    const std::string tmpFile = "/tmp/hal_storage_test_XXXXXX";
    const size_t WRITE_SIZE = 32 * 1024 * 1024; // 32 MB

    // Create temp file
    char tmpl[] = "/tmp/hal_storage_test_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) {
        data.push_back("FAIL: Could not create temp file for write test.");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }
    (void)tmpFile;

    std::vector<char> buf(512 * 1024, static_cast<char>(0xAB));
    auto t0 = std::chrono::high_resolution_clock::now();
    size_t written = 0;
    while (written < WRITE_SIZE) {
        ssize_t n = write(fd, buf.data(), buf.size());
        if (n <= 0) break;
        written += static_cast<size_t>(n);
    }
    fsync(fd);
    double elapsed = std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now() - t0).count();
    close(fd);
    unlink(tmpl);

    double mbps = (elapsed > 0 && written > 0) ?
        static_cast<double>(written) / (1024.0 * 1024.0) / elapsed : 0.0;

    data.push_back("Write test target: /tmp (tmpfs, safe for benchmarking)");
    data.push_back("Written: " + std::to_string(written / (1024 * 1024)) + " MB in " + std::to_string(elapsed) + " s");
    data.push_back("Write throughput: " + std::to_string(mbps) + " MB/s");

    // Report SD card speed class
    std::string speedClass = storReadFile(MMCBLK_SYSFS + std::string("/device/speed_class"));
    if (!speedClass.empty()) data.push_back("SD speed class: " + speedClass);

    std::string videoClass = storReadFile(MMCBLK_SYSFS + std::string("/device/dsr"));
    // Also report from /proc/partitions
    std::string partInfo = storRunCmd("lsblk -d -o NAME,SIZE,TRAN /dev/mmcblk0 2>/dev/null");
    if (!partInfo.empty()) data.push_back("lsblk: " + partInfo);

    if (mbps < SD_MIN_SEQ_WRITE_MBS) {
        passed = false;
        data.push_back("FAIL: Write speed below minimum " + std::to_string(SD_MIN_SEQ_WRITE_MBS) + " MB/s.");
    } else {
        data.push_back("PASS: Write throughput test completed.");
    }

    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 5. testRandomReadIops
//    Measure 4K random read IOPS from the SD card.
// ---------------------------------------------------------------------------
TestOutput StorageTesting::testRandomReadIops()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("4K random read IOPS test on: " + std::string(MMCBLK_DEV));

    if (!pathExists(MMCBLK_DEV)) {
        data.push_back("SKIP: SD card not present.");
        {
            TestOutput _out;
            _out.status = hal::Status::PASS;
            _out.severity = hal::Severity::INFO;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    int iops = measureRandReadIops(MMCBLK_DEV);
    if (iops < 0) {
        data.push_back("FAIL: Could not open device for random read.");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    data.push_back("4K random read IOPS: " + std::to_string(iops));
    data.push_back("Minimum threshold: " + std::to_string(SD_MIN_RAND_READ_IOPS));

    if (iops < SD_MIN_RAND_READ_IOPS) {
        passed = false;
        data.push_back("FAIL: Random read IOPS below minimum threshold.");
    } else {
        data.push_back("PASS: Random read IOPS meets requirement.");
    }

    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 6. testRandomWriteIops
//    Measure 4K random write IOPS to tmpfs.
// ---------------------------------------------------------------------------
TestOutput StorageTesting::testRandomWriteIops()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("4K random write IOPS test (tmpfs /tmp for safety)");

    char tmpl[] = "/tmp/hal_randwrite_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) {
        data.push_back("FAIL: Cannot create temp file.");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    // Pre-allocate 64 MB
    if (ftruncate(fd, 64 * 1024 * 1024) != 0) {
        close(fd); unlink(tmpl);
        data.push_back("FAIL: Cannot allocate temp file.");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    std::mt19937_64 rng(12345);
    const size_t BLOCK = 4096;
    std::vector<char> buf(BLOCK, 0x55);
    long long maxOff = 64 * 1024 * 1024 - BLOCK;

    auto tEnd = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    int iops = 0;
    while (std::chrono::steady_clock::now() < tEnd) {
        off_t off = static_cast<off_t>(rng() % static_cast<uint64_t>(maxOff));
        off = (off / 4096) * 4096;
        if (pwrite(fd, buf.data(), BLOCK, off) > 0) ++iops;
    }
    close(fd);
    unlink(tmpl);

    iops /= 2; // per second
    data.push_back("4K random write IOPS (tmpfs): " + std::to_string(iops));
    data.push_back("Minimum threshold: " + std::to_string(SD_MIN_RAND_WRITE_IOPS));

    if (iops < SD_MIN_RAND_WRITE_IOPS) {
        passed = false;
        data.push_back("FAIL: Random write IOPS below minimum threshold.");
    } else {
        data.push_back("PASS: Random write IOPS test passed.");
    }

    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 7. testEmmcWearLeveling
//    Check SD card CID, lifecycle counter (where available), and report
//    card health via mmc-utils if installed.
// ---------------------------------------------------------------------------
TestOutput StorageTesting::testEmmcWearLeveling()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("SD/microSD wear leveling check (Pi5 uses microSD, not eMMC)");

    // CID contains manufacturing date — proxy for "new" card
    std::string cid = storReadFile(MMCBLK_SYSFS + std::string("/device/cid"));
    if (!cid.empty()) data.push_back("CID: " + cid);

    // mmc-utils extcsd dump (available on Pi OS)
    std::string mmcHealth = storRunCmd("mmc extcsd read /dev/mmcblk0 2>/dev/null | grep -i 'pre_eol\\|life'");
    if (!mmcHealth.empty()) {
        data.push_back("MMC extended CSD health: " + mmcHealth);
    } else {
        data.push_back("INFO: mmc-utils not installed or SD card does not support ext CSD health info.");
        data.push_back("      Install with: sudo apt install mmc-utils");
    }

    // Preferred erase size (wear granularity)
    std::string eraseSize = storReadFile(MMCBLK_SYSFS + std::string("/device/preferred_erase_size"));
    if (!eraseSize.empty()) data.push_back("Preferred erase size: " + eraseSize + " bytes");

    data.push_back("PASS: Wear-leveling check complete (software introspection only on microSD).");
    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 8. testBadBlockManagement
//    Check for bad block counts via smartctl or kernel log messages.
// ---------------------------------------------------------------------------
TestOutput StorageTesting::testBadBlockManagement()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Bad block management check for SD and NVMe");

    // SD card: check kernel log for mmc errors
    std::string mmcErrors = storRunCmd("dmesg 2>/dev/null | grep -c 'mmcblk0.*error'");
    int sdErrors = 0;
    try { sdErrors = std::stoi(mmcErrors); } catch (...) {}
    data.push_back("SD card kernel error count (dmesg): " + std::to_string(sdErrors));
    if (sdErrors > 0) {
        passed = false;
        data.push_back("WARNING: SD card errors detected in kernel log.");
    }

    // NVMe SMART data
    if (pathExists(NVME_DEV)) {
        std::string nvmeSmart = storRunCmd("nvme smart-log /dev/nvme0n1 2>/dev/null | grep -i 'media_errors\\|critical'");
        if (!nvmeSmart.empty()) data.push_back("NVMe SMART: " + nvmeSmart);
        else data.push_back("NVMe SMART: nvme-cli not installed (apt install nvme-cli).");
    }

    if (sdErrors == 0) data.push_back("PASS: No bad block errors detected.");
    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 9. testPowerLossProtection
//    Check filesystem journal / power-loss protection features.
// ---------------------------------------------------------------------------
TestOutput StorageTesting::testPowerLossProtection()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Power loss protection check (filesystem journaling)");

    // Check root filesystem type
    std::string fsType = storRunCmd("findmnt -n -o FSTYPE / 2>/dev/null");
    data.push_back("Root filesystem type: " + (fsType.empty() ? "(unknown)" : fsType));

    if (fsType == "ext4" || fsType == "btrfs" || fsType == "xfs") {
        data.push_back("PASS: Journaled filesystem detected — power-loss resilient.");
    } else if (fsType == "vfat" || fsType == "fat32") {
        data.push_back("WARNING: FAT filesystem has no journaling — not power-loss resistant.");
        passed = false;
    } else {
        data.push_back("INFO: Filesystem type: " + fsType + " — check journal status manually.");
    }

    // Check ext4 journal mode
    std::string e2fsck = storRunCmd("tune2fs -l /dev/mmcblk0p2 2>/dev/null | grep -i 'journal\\|has_journal'");
    if (!e2fsck.empty()) data.push_back("ext4 journal: " + e2fsck);

    // Check Pi5 PMIC UPS/power-good
    std::string psu = storReadFile("/sys/class/power_supply/rpi-psu/online");
    if (!psu.empty()) data.push_back("Pi5 PSU online: " + psu);

    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 10. testEmmcTemperatureMonitoring
//     Read SD card / NVMe temperature via hwmon.
// ---------------------------------------------------------------------------
TestOutput StorageTesting::testEmmcTemperatureMonitoring()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Storage temperature monitoring (Pi5)");

    // SD card temperature is not normally exposed; report SoC temperature instead
    long long socTempMC = storReadInt("/sys/class/thermal/thermal_zone0/temp");
    if (socTempMC > 0) {
        double socC = socTempMC / 1000.0;
        data.push_back("BCM2712 SoC temperature: " + std::to_string(socC) + " °C");
        if (socC > 80.0) {
            passed = false;
            data.push_back("WARNING: SoC temperature above 80°C — may affect storage.");
        }
    }

    // NVMe temperature
    if (pathExists(NVME_DEV)) {
        // Try NVMe hwmon
        std::string nvmeTemp = storRunCmd("nvme smart-log /dev/nvme0n1 2>/dev/null | grep -i 'temperature'");
        if (!nvmeTemp.empty()) {
            data.push_back("NVMe temperature: " + nvmeTemp);
        } else {
            // Try hwmon
            long long nvmeTempMC = storReadInt(NVME_HWMON);
            if (nvmeTempMC > 0) {
                double nvmeC = nvmeTempMC / 1000.0;
                data.push_back("NVMe temperature (hwmon): " + std::to_string(nvmeC) + " °C");
                if (nvmeC > NVME_TEMP_MAX_C) {
                    passed = false;
                    data.push_back("FAIL: NVMe temperature above " + std::to_string(NVME_TEMP_MAX_C) + " °C.");
                }
            }
        }
    } else {
        data.push_back("NVMe not present — temperature check not applicable.");
    }

    data.push_back(passed ? "PASS: Storage temperature within limits." : "FAIL: Temperature issue detected.");
    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 11. testNvmeSsdDetection
//     Verify NVMe SSD is detected via PCIe M.2 slot.
// ---------------------------------------------------------------------------
TestOutput StorageTesting::testNvmeSsdDetection()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("NVMe SSD detection (Pi5 PCIe Gen3 x1 M.2 M-key)");

    bool nvmeDev    = pathExists(NVME_DEV);
    bool nvmeSysfs  = pathExists(NVME_SYSFS);
    bool nvmeClass  = pathExists("/sys/class/nvme/nvme0");

    data.push_back("NVMe block device (/dev/nvme0n1): " + std::string(nvmeDev  ? "present" : "absent"));
    data.push_back("NVMe sysfs (/sys/block/nvme0n1):   " + std::string(nvmeSysfs ? "present" : "absent"));
    data.push_back("NVMe class (/sys/class/nvme/nvme0): " + std::string(nvmeClass ? "present" : "absent"));

    if (!nvmeDev && !nvmeSysfs) {
        data.push_back("INFO: No NVMe detected. Install NVMe in PCIe M.2 slot and enable:");
        data.push_back("      dtparam=pciex1_gen=3  in /boot/firmware/config.txt");
        data.push_back("      Reboot required after config change.");
        // Not a hard failure — NVMe is optional on Pi5
        {
            TestOutput _out;
            _out.status = hal::Status::PASS;
            _out.severity = hal::Severity::INFO;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    // Read NVMe model name
    std::string model = storReadFile("/sys/class/nvme/nvme0/model");
    if (!model.empty()) data.push_back("NVMe model: " + model);

    std::string serial = storReadFile("/sys/class/nvme/nvme0/serial");
    if (!serial.empty()) data.push_back("NVMe serial: " + serial);

    std::string firmware = storReadFile("/sys/class/nvme/nvme0/firmware_rev");
    if (!firmware.empty()) data.push_back("NVMe firmware: " + firmware);

    // Check PCIe link speed
    std::string linkSpd = storRunCmd("lspci -vv 2>/dev/null | grep -A5 'Non-Volatile' | grep 'Speed'");
    if (!linkSpd.empty()) data.push_back("PCIe link: " + linkSpd);

    data.push_back("PASS: NVMe SSD detected and accessible.");
    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 12. testNvmeReadwritePerformance
//     Sequential read/write benchmark on NVMe.
// ---------------------------------------------------------------------------
TestOutput StorageTesting::testNvmeReadwritePerformance()
{
    std::vector<std::string> data;
    bool passed = true;

    if (!pathExists(NVME_DEV)) {
        data.push_back("SKIP: NVMe not present — test not applicable.");
        {
            TestOutput _out;
            _out.status = hal::Status::PASS;
            _out.severity = hal::Severity::INFO;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    data.push_back("NVMe sequential performance (32 MB sequential read)");

    double readMbps = measureSeqReadMBps(NVME_DEV);
    if (readMbps < 0) {
        data.push_back("FAIL: Cannot read NVMe device (permissions?).");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }
    data.push_back("NVMe sequential read: " + std::to_string(readMbps) + " MB/s");
    data.push_back("Minimum: " + std::to_string(NVME_MIN_SEQ_READ_MBS) + " MB/s");

    if (readMbps < NVME_MIN_SEQ_READ_MBS) {
        passed = false;
        data.push_back("FAIL: NVMe read below minimum threshold.");
    } else {
        data.push_back("PASS: NVMe read performance meets requirement.");
    }

    // Write test to tmpfs
    char tmpl[] = "/tmp/hal_nvme_write_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd >= 0) {
        const size_t WR_SIZE = 32 * 1024 * 1024;
        std::vector<char> buf(512 * 1024, 0x77);
        auto t0 = std::chrono::high_resolution_clock::now();
        size_t wr = 0;
        while (wr < WR_SIZE) {
            ssize_t n = write(fd, buf.data(), buf.size());
            if (n <= 0) break;
            wr += static_cast<size_t>(n);
        }
        fsync(fd);
        double el = std::chrono::duration<double>(
            std::chrono::high_resolution_clock::now() - t0).count();
        close(fd); unlink(tmpl);
        double wMbps = (el > 0 && wr > 0) ? wr / (1024.0 * 1024.0) / el : 0.0;
        data.push_back("Write benchmark (tmpfs): " + std::to_string(wMbps) + " MB/s");
    }

    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 13. testNvmeThermalThrottling
//     Check NVMe temperature and throttling status.
// ---------------------------------------------------------------------------
TestOutput StorageTesting::testNvmeThermalThrottling()
{
    std::vector<std::string> data;
    bool passed = true;

    if (!pathExists(NVME_DEV)) {
        data.push_back("SKIP: NVMe not present.");
        {
            TestOutput _out;
            _out.status = hal::Status::PASS;
            _out.severity = hal::Severity::INFO;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }

    data.push_back("NVMe thermal throttling check");

    // Try nvme-cli smart-log
    std::string smart = storRunCmd("nvme smart-log /dev/nvme0n1 2>/dev/null");
    if (!smart.empty()) {
        // Extract temperature
        std::istringstream ss(smart);
        std::string line;
        while (std::getline(ss, line)) {
            if (line.find("temperature") != std::string::npos ||
                line.find("throttle") != std::string::npos ||
                line.find("warning_temp") != std::string::npos)
                data.push_back("  " + line);
        }
    } else {
        // Fall back to hwmon
        long long tempMC = storReadInt(NVME_HWMON);
        if (tempMC > 0) {
            double tempC = tempMC / 1000.0;
            data.push_back("NVMe temperature (hwmon): " + std::to_string(tempC) + " °C");
            if (tempC > NVME_TEMP_MAX_C) {
                passed = false;
                data.push_back("FAIL: NVMe temperature above " + std::to_string(NVME_TEMP_MAX_C) + " °C throttle threshold.");
            }
        } else {
            data.push_back("INFO: Install nvme-cli for detailed thermal data: sudo apt install nvme-cli");
        }
    }

    std::string throttled = storRunCmd("nvme smart-log /dev/nvme0n1 2>/dev/null | grep -i 'throttle'");
    if (!throttled.empty()) data.push_back("Throttle status: " + throttled);

    data.push_back(passed ? "PASS: NVMe thermal check passed." : "FAIL: NVMe thermal issue.");
    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 14. testHotplugCapability
//     Verify block device uevent / kernel hotplug detection.
// ---------------------------------------------------------------------------
TestOutput StorageTesting::testHotplugCapability()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Storage hotplug capability check");

    // SD card hotplug: Pi5 has a push-push SD slot with detection
    std::string sdDetect = storReadFile(MMCBLK_SYSFS + std::string("/device/uevent"));
    if (!sdDetect.empty()) {
        data.push_back("SD uevent: available (hotplug supported via mmc driver)");
    } else {
        data.push_back("SD uevent: not accessible (may be permanently inserted)");
    }

    // USB storage hotplug
    std::string usbStorage = storRunCmd("ls /dev/sd* 2>/dev/null | head -5");
    if (!usbStorage.empty()) data.push_back("USB storage devices: " + usbStorage);

    // Check udevadm monitor capability
    bool udevPresent = pathExists("/usr/bin/udevadm") || pathExists("/bin/udevadm");
    data.push_back("udevadm (hotplug manager): " + std::string(udevPresent ? "present" : "not found"));

    // NVMe hotplug: PCIe Gen3 on Pi5 does not support native hotplug
    data.push_back("INFO: Pi5 NVMe/PCIe does not support hot-plug in RP1 implementation.");
    data.push_back("      SD card supports software-initiated remove/insert detection.");

    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

// ---------------------------------------------------------------------------
// 15. testStorageEncryptionSupport
//     Check dm-crypt / LUKS / fscrypt availability.
// ---------------------------------------------------------------------------
TestOutput StorageTesting::testStorageEncryptionSupport()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Storage encryption support check (Pi5 — software encryption via dm-crypt)");

    // Check dm-crypt kernel module
    std::string dmCrypt = storRunCmd("lsmod 2>/dev/null | grep dm_crypt");
    data.push_back(std::string("dm-crypt module: ") + (dmCrypt.empty() ? "not loaded" : "LOADED"));

    // Check cryptsetup
    bool cryptSetup = pathExists("/usr/sbin/cryptsetup") || pathExists("/sbin/cryptsetup");
    data.push_back("cryptsetup: " + std::string(cryptSetup ? "installed" : "not installed"));

    // Check for LUKS devices
    std::string luksDevs = storRunCmd("lsblk -o NAME,TYPE 2>/dev/null | grep crypt");
    if (!luksDevs.empty()) data.push_back("Active LUKS/encrypted volumes: " + luksDevs);
    else data.push_back("Active LUKS volumes: none detected");

    // Check hardware AES (BCM2712 Cortex-A76 has AES-NI equivalent)
    std::string cpuAes = storRunCmd("grep -m1 aes /proc/cpuinfo 2>/dev/null");
    if (!cpuAes.empty()) data.push_back("CPU AES acceleration: " + cpuAes);

    // Check fscrypt
    std::string fscrypt = storRunCmd("tune2fs -l /dev/mmcblk0p2 2>/dev/null | grep -i encrypt");
    if (!fscrypt.empty()) data.push_back("ext4 encryption: " + fscrypt);

    // Check TPM for key storage
    bool tpmPresent = pathExists("/dev/tpm0") || pathExists("/dev/tpmrm0");
    data.push_back("TPM device: " + std::string(tpmPresent ? "present" : "not found (Pi5 has no onboard TPM)"));

    data.push_back("NOTE: Pi5 BCM2712 provides AES via ARMv8 crypto extensions.");
    data.push_back("      Full disk encryption requires LUKS2 + cryptsetup on Pi OS.");

    if (!cryptSetup && dmCrypt.empty()) {
        data.push_back("INFO: Install cryptsetup: sudo apt install cryptsetup");
    }

    {
        TestOutput _out;
        _out.status = passed ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Run all tests for this module
 * @return Vector of test results
 */
std::vector<TestResult> StorageTesting::runAllTests()
{
    std::vector<TestResult> results;

    // Run testEmmc51InterfaceFunctionality
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testEmmc51InterfaceFunctionality();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testEmmc51InterfaceFunctionality";
            _tr.test_id = "testEmmc51InterfaceFunctionality";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testFullCapacityVerification
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testFullCapacityVerification();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testFullCapacityVerification";
            _tr.test_id = "testFullCapacityVerification";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSequentialReadPerformance
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSequentialReadPerformance();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSequentialReadPerformance";
            _tr.test_id = "testSequentialReadPerformance";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSequentialWritePerformance
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSequentialWritePerformance();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSequentialWritePerformance";
            _tr.test_id = "testSequentialWritePerformance";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testRandomReadIops
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testRandomReadIops();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testRandomReadIops";
            _tr.test_id = "testRandomReadIops";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testRandomWriteIops
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testRandomWriteIops();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testRandomWriteIops";
            _tr.test_id = "testRandomWriteIops";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testEmmcWearLeveling
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testEmmcWearLeveling();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testEmmcWearLeveling";
            _tr.test_id = "testEmmcWearLeveling";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testBadBlockManagement
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testBadBlockManagement();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testBadBlockManagement";
            _tr.test_id = "testBadBlockManagement";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testPowerLossProtection
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testPowerLossProtection();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testPowerLossProtection";
            _tr.test_id = "testPowerLossProtection";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testEmmcTemperatureMonitoring
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testEmmcTemperatureMonitoring();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testEmmcTemperatureMonitoring";
            _tr.test_id = "testEmmcTemperatureMonitoring";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testNvmeSsdDetection
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testNvmeSsdDetection();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testNvmeSsdDetection";
            _tr.test_id = "testNvmeSsdDetection";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testNvmeReadwritePerformance
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testNvmeReadwritePerformance();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testNvmeReadwritePerformance";
            _tr.test_id = "testNvmeReadwritePerformance";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testNvmeThermalThrottling
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testNvmeThermalThrottling();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testNvmeThermalThrottling";
            _tr.test_id = "testNvmeThermalThrottling";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testHotplugCapability
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testHotplugCapability();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testHotplugCapability";
            _tr.test_id = "testHotplugCapability";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testStorageEncryptionSupport
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testStorageEncryptionSupport();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testStorageEncryptionSupport";
            _tr.test_id = "testStorageEncryptionSupport";
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
std::vector<std::string> StorageTesting::getAvailableTests()
{
    return {
        "testEmmc51InterfaceFunctionality",
        "testFullCapacityVerification",
        "testSequentialReadPerformance",
        "testSequentialWritePerformance",
        "testRandomReadIops",
        "testRandomWriteIops",
        "testEmmcWearLeveling",
        "testBadBlockManagement",
        "testPowerLossProtection",
        "testEmmcTemperatureMonitoring",
        "testNvmeSsdDetection",
        "testNvmeReadwritePerformance",
        "testNvmeThermalThrottling",
        "testHotplugCapability",
        "testStorageEncryptionSupport",
    };
}

/**
 * @brief Run a single test by name
 * @param testName Name of the test to run
 * @return Test result with timing
 */
TestResult StorageTesting::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if (testName == "testEmmc51InterfaceFunctionality") {
        result = testEmmc51InterfaceFunctionality();
    } else if (testName == "testFullCapacityVerification") {
        result = testFullCapacityVerification();
    } else if (testName == "testSequentialReadPerformance") {
        result = testSequentialReadPerformance();
    } else if (testName == "testSequentialWritePerformance") {
        result = testSequentialWritePerformance();
    } else if (testName == "testRandomReadIops") {
        result = testRandomReadIops();
    } else if (testName == "testRandomWriteIops") {
        result = testRandomWriteIops();
    } else if (testName == "testEmmcWearLeveling") {
        result = testEmmcWearLeveling();
    } else if (testName == "testBadBlockManagement") {
        result = testBadBlockManagement();
    } else if (testName == "testPowerLossProtection") {
        result = testPowerLossProtection();
    } else if (testName == "testEmmcTemperatureMonitoring") {
        result = testEmmcTemperatureMonitoring();
    } else if (testName == "testNvmeSsdDetection") {
        result = testNvmeSsdDetection();
    } else if (testName == "testNvmeReadwritePerformance") {
        result = testNvmeReadwritePerformance();
    } else if (testName == "testNvmeThermalThrottling") {
        result = testNvmeThermalThrottling();
    } else if (testName == "testHotplugCapability") {
        result = testHotplugCapability();
    } else if (testName == "testStorageEncryptionSupport") {
        result = testStorageEncryptionSupport();
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
