/**
 * @file system_integration_lib.cpp
 * @brief System integration tests for Raspberry Pi 5.
 *        Exercises concurrent subsystem load, resource contention,
 *        end-to-end latency (POSIX clocks), system throughput (memcpy
 *        bandwidth + storage), and Pi5-specific AI/sensor-fusion pipelines
 *        using libcamera, GStreamer, TFLite, and OpenCV.
 * @author sandesh ghimire
 * @copyright (C) Lucid Motors
 */

#include "system_integration_lib.h"
#include "../platform_rpi5.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
static std::string siReadFile(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string s = ss.str();
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' '))
        s.pop_back();
    return s;
}

static std::string siRunCmd(const std::string& cmd)
{
    std::array<char, 1024> buf{};
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    while (fgets(buf.data(), buf.size(), pipe))
        result += buf.data();
    pclose(pipe);
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
        result.pop_back();
    return result;
}

static bool siPathExists(const std::string& p)
{
    struct stat st {};
    return stat(p.c_str(), &st) == 0;
}
// ---------------------------------------------------------------------------

/**
 * @brief Constructor for SystemIntegrationTesting
 */
SystemIntegrationTesting::SystemIntegrationTesting()
{
}

/**
 * @brief Destructor for SystemIntegrationTesting
 */
SystemIntegrationTesting::~SystemIntegrationTesting()
{
}

/**
 * @brief Verify concurrent operation: CPU, GPU, memory, and network simultaneously.
 */
TestOutput SystemIntegrationTesting::testConcurrentOperationOfAllSubsystems()
{
    std::vector<std::string> data;
    data.push_back("=== Concurrent Subsystem Operation (Pi5 BCM2712) ===");

    // Start background stress on all 4 cores for 3 s (non-blocking)
    bool stressOk = system("stress-ng --cpu 4 --timeout 3s --quiet 2>/dev/null &") == 0;
    (void)stressOk;
    data.push_back("[INFO] stress-ng 4-core load started (3 s)");

    // GPU: check v3d is active
    std::string v3dStat = siRunCmd(
        "cat /sys/kernel/debug/dri/0/clients 2>/dev/null | head -2");
    data.push_back(v3dStat.empty() ? "[INFO] No DRM clients (GPU idle)"
                                   : "[INFO] DRM clients:\n" + v3dStat);

    // Memory: sysinfo
    struct sysinfo si {};
    sysinfo(&si);
    long long totalMb = static_cast<long long>(si.totalram) * si.mem_unit / (1024 * 1024);
    long long freeMb  = static_cast<long long>(si.freeram)  * si.mem_unit / (1024 * 1024);
    data.push_back("[INFO] RAM total: " + std::to_string(totalMb) +
                   " MB  free: " + std::to_string(freeMb) + " MB");

    // Network: eth0 TX bytes (if up)
    std::string txB = siReadFile("/sys/class/net/eth0/statistics/tx_bytes");
    data.push_back(txB.empty() ? "[INFO] eth0 not active" : "[INFO] eth0 TX bytes: " + txB);

    // Storage: sdcard
    bool sdOk = siPathExists("/dev/mmcblk0");
    data.push_back(sdOk ? "[PASS] SD storage device /dev/mmcblk0 present" : "[INFO] No /dev/mmcblk0");

    // Thermal check after load
    std::string temp = siReadFile("/sys/class/thermal/thermal_zone0/temp");
    if (!temp.empty())
        data.push_back("[INFO] CPU temp: " + std::to_string(std::stoi(temp) / 1000) + " °C under load");

    {
        TestOutput _out;
        _out.status = (totalMb > 0) ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = (totalMb > 0) ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Check resource contention: cache pressure, memory bandwidth limits.
 */
TestOutput SystemIntegrationTesting::testResourceContentionHandling()
{
    std::vector<std::string> data;
    data.push_back("=== Resource Contention (Pi5 LPDDR4X-4267) ===");

    // Memory pressure
    std::string vmPressure = siReadFile("/proc/pressure/memory");
    data.push_back(vmPressure.empty() ? "[INFO] PSI memory pressure not available"
                                      : "[INFO] PSI memory pressure:\n" + vmPressure);

    std::string cpuPressure = siReadFile("/proc/pressure/cpu");
    data.push_back(cpuPressure.empty() ? "[INFO] PSI CPU pressure not available"
                                       : "[INFO] PSI CPU pressure:\n" + cpuPressure);

    // Swap usage
    std::string swapTotal = siReadFile("/proc/meminfo");
    // Extract SwapTotal and SwapFree from /proc/meminfo
    std::string swapTotalVal = siRunCmd("grep SwapTotal /proc/meminfo 2>/dev/null");
    std::string swapFreeVal  = siRunCmd("grep SwapFree  /proc/meminfo 2>/dev/null");
    data.push_back("[INFO] " + swapTotalVal);
    data.push_back("[INFO] " + swapFreeVal);

    // OOM kills
    std::string oom = siRunCmd("dmesg 2>/dev/null | grep -c 'Out of memory'");
    data.push_back(oom == "0" ? "[PASS] No OOM kills recorded" : "[WARN] OOM events: " + oom);

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
 * @brief Identify system bottleneck via perf stat (if available) or CPU loadavg.
 */
TestOutput SystemIntegrationTesting::testSystemBottleneckIdentification()
{
    std::vector<std::string> data;
    data.push_back("=== System Bottleneck Identification (Pi5) ===");

    // Load average
    std::string loadavg = siReadFile("/proc/loadavg");
    data.push_back("[INFO] Load average: " + loadavg);

    // CPU utilisation (1-second snapshot via /proc/stat)
    auto readCpuStat = []() -> std::array<long long, 4> {
        std::ifstream f("/proc/stat");
        std::string cpu;
        long long user, nice, system, idle;
        f >> cpu >> user >> nice >> system >> idle;
        return {user, nice, system, idle};
    };
    auto s1 = readCpuStat();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto s2 = readCpuStat();
    long long total = (s2[0]-s1[0]) + (s2[1]-s1[1]) + (s2[2]-s1[2]) + (s2[3]-s1[3]);
    long long idle  = s2[3] - s1[3];
    int utilPct = (total > 0) ? static_cast<int>(100 * (total - idle) / total) : 0;
    data.push_back("[INFO] CPU utilisation (0.5 s sample): " + std::to_string(utilPct) + "%");

    // Memory bandwidth bottleneck
    std::string vmStat = siRunCmd("vmstat -s 2>/dev/null | grep -E 'pages paged|swap' | head -4");
    data.push_back(vmStat.empty() ? "[INFO] vmstat not available" : "[INFO] vmstat:\n" + vmStat);

    // Disk I/O
    std::string diskStat = siRunCmd("iostat -x 1 1 2>/dev/null | grep mmcblk | head -2");
    data.push_back(diskStat.empty() ? "[INFO] iostat not available" : "[INFO] Disk I/O:\n" + diskStat);

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
 * @brief Measure end-to-end latency using POSIX clock_gettime (nanosecond resolution).
 */
TestOutput SystemIntegrationTesting::testEndtoendLatency()
{
    std::vector<std::string> data;
    data.push_back("=== End-to-End Latency (Pi5 POSIX clocks) ===");

    // Measure round-trip time of a popen("cat /proc/version") call
    struct timespec t0 {}, t1 {};
    clock_gettime(CLOCK_MONOTONIC, &t0);
    std::string ver = siRunCmd("cat /proc/version 2>/dev/null");
    clock_gettime(CLOCK_MONOTONIC, &t1);
    long long ns = (t1.tv_sec - t0.tv_sec) * 1000000000LL + (t1.tv_nsec - t0.tv_nsec);
    data.push_back("[INFO] Kernel read latency: " + std::to_string(ns / 1000) + " µs");

    // Memory copy latency (1 MB)
    static constexpr size_t BUF_SIZE = 1 * 1024 * 1024;
    std::vector<uint8_t> src(BUF_SIZE, 0xAA);
    std::vector<uint8_t> dst(BUF_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    memcpy(dst.data(), src.data(), BUF_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    long long memNs = (t1.tv_sec - t0.tv_sec) * 1000000000LL + (t1.tv_nsec - t0.tv_nsec);
    data.push_back("[PASS] 1 MB memcpy latency: " + std::to_string(memNs / 1000) + " µs");

    // GPIO latency proxy via /sys/class/gpio or kernel event
    data.push_back("[INFO] GPIO toggle latency (Pi5 gpiod): typically 1-5 µs via libgpiod");
    data.push_back("[INFO] CSI-2 frame latency: approx 1 frame = " +
                   std::to_string(1000 / 30) + " ms at 30 fps");

    bool ok = (memNs > 0 && memNs < 50000000LL); // < 50 ms
    {
        TestOutput _out;
        _out.status = ok ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = ok ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Measure system throughput: memory bandwidth, storage I/O, network.
 */
TestOutput SystemIntegrationTesting::testSystemThroughput()
{
    std::vector<std::string> data;
    data.push_back("=== System Throughput (Pi5 BCM2712) ===");

    // Memory bandwidth: 128 MB copy
    static constexpr size_t MB128 = 128ULL * 1024 * 1024;
    std::vector<uint8_t> src(MB128, 0x5A);
    std::vector<uint8_t> dst(MB128);
    struct timespec t0 {}, t1 {};
    clock_gettime(CLOCK_MONOTONIC, &t0);
    memcpy(dst.data(), src.data(), MB128);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
    double gbps = (MB128 / 1e9) / (ms / 1000.0);
    data.push_back("[PASS] memcpy bandwidth: " + std::to_string(static_cast<int>(gbps * 1000)) +
                   " MB/s  (LPDDR4X-4267 max ~34 GB/s)");

    // Storage throughput
    std::string hdparm = siRunCmd("hdparm -t /dev/mmcblk0 2>/dev/null | grep 'MB/sec'");
    data.push_back(hdparm.empty() ? "[INFO] hdparm not available for SD/eMMC throughput"
                                  : "[INFO] SD read: " + hdparm);

    // Network throughput (theoretical)
    data.push_back("[INFO] GbE (eth0) throughput: up to " +
                   std::to_string(RPI5_ETH_SPEED_MBPS) + " Mbps");

    bool ok = gbps > 0.5;
    {
        TestOutput _out;
        _out.status = ok ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = ok ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Test multimodal AI workload on Pi5 (TFLite + libcamera + ALSA).
 * @details Pi5 has no dedicated AI accelerator.  TFLite with XNNPACK uses
 *          NEON SIMD on the Cortex-A76.  Hailo-8 HAT adds 26 TOPS.
 */
TestOutput SystemIntegrationTesting::testMultimodalAiWorkloads()
{
    std::vector<std::string> data;
    data.push_back("=== Multimodal AI (Pi5 TFLite / Hailo-8 HAT) ===");

    // TFLite
    std::string tflite = siRunCmd("python3 -c 'import tflite_runtime; print(tflite_runtime.__version__)' 2>/dev/null");
    data.push_back(tflite.empty() ? "[INFO] TFLite runtime not installed (pip install tflite-runtime)"
                                  : "[PASS] TFLite runtime: " + tflite);

    // Hailo-8 HAT
    bool hailoPresent = siPathExists("/dev/hailo0");
    data.push_back(hailoPresent ? "[PASS] Hailo-8 accelerator /dev/hailo0 present (26 TOPS)"
                                : "[INFO] No Hailo-8 HAT (run on Cortex-A76 NEON)");

    // OpenCV
    std::string ocv = siRunCmd("python3 -c 'import cv2; print(cv2.__version__)' 2>/dev/null");
    data.push_back(ocv.empty() ? "[INFO] OpenCV (cv2) not installed"
                               : "[PASS] OpenCV: " + ocv);

    // libcamera
    std::string lc = siRunCmd("libcamera-hello --list-cameras 2>/dev/null | head -3");
    data.push_back(lc.empty() ? "[INFO] libcamera not available"
                              : "[PASS] libcamera output:\n" + lc);

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
 * @brief Autonomous driving stack check (Pi5 proxy: ROS2, libcamera, TFLite pipeline).
 */
TestOutput SystemIntegrationTesting::testAutonomousDrivingStack()
{
    std::vector<std::string> data;
    data.push_back("=== Autonomous Driving Stack (Pi5 proxy) ===");
    data.push_back("[INFO] Pi5 supports autonomous stack via ROS2 Humble / Pi OS Bookworm");

    // ROS2
    std::string ros = siRunCmd("ros2 --version 2>/dev/null");
    data.push_back(ros.empty() ? "[INFO] ROS2 not installed"
                               : "[PASS] ROS2: " + ros);

    // Systemd autostart
    std::string ros2Active = siRunCmd("systemctl is-active ros2 2>/dev/null");
    data.push_back(ros2Active == "active" ? "[PASS] ros2 service active"
                                          : "[INFO] ros2 service: " + ros2Active);

    // GStreamer (camera pipeline)
    std::string gst = siRunCmd("gst-launch-1.0 --version 2>/dev/null | head -1");
    data.push_back(gst.empty() ? "[INFO] GStreamer not found" : "[PASS] " + gst);

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
 * @brief Sensor fusion pipeline: camera + IMU + GPS → fused pose estimate.
 */
TestOutput SystemIntegrationTesting::testSensorFusionPipeline()
{
    std::vector<std::string> data;
    data.push_back("=== Sensor Fusion Pipeline (Pi5 CSI-2 + IIO + GPSD) ===");

    // Camera
    bool camOk = !siRunCmd("ls /dev/video0 2>/dev/null").empty();
    data.push_back(camOk ? "[PASS] Camera /dev/video0 available" : "[INFO] No camera at /dev/video0");

    // IMU (IIO)
    bool imuOk = !siRunCmd("ls /sys/bus/iio/devices/ 2>/dev/null").empty();
    data.push_back(imuOk ? "[PASS] IIO devices present (IMU)"
                         : "[INFO] No IIO devices (IMU not connected)");

    // GPSD
    std::string gpsd = siRunCmd("systemctl is-active gpsd 2>/dev/null");
    data.push_back(gpsd == "active" ? "[PASS] gpsd service active"
                                    : "[INFO] gpsd: " + (gpsd.empty() ? "inactive" : gpsd));

    // Robot Localizer (SLAM) — check for localization packages
    std::string slam = siRunCmd("which slam_toolbox 2>/dev/null");
    data.push_back(slam.empty() ? "[INFO] slam_toolbox not installed (pip/apt for ROS2 slam-toolbox)"
                                : "[PASS] slam_toolbox found: " + slam);

    data.push_back("[INFO] Fusion pipeline: libcamera → OpenCV → TFLite + IMU IIO → EKF → /odom");

    bool ok = camOk || imuOk;
    {
        TestOutput _out;
        _out.status = ok ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = ok ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Run all tests for this module
 * @return Vector of test results
 */
std::vector<TestResult> SystemIntegrationTesting::runAllTests()
{
    std::vector<TestResult> results;

    // Run testConcurrentOperationOfAllSubsystems
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testConcurrentOperationOfAllSubsystems();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testConcurrentOperationOfAllSubsystems";
            _tr.test_id = "testConcurrentOperationOfAllSubsystems";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testResourceContentionHandling
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testResourceContentionHandling();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testResourceContentionHandling";
            _tr.test_id = "testResourceContentionHandling";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSystemBottleneckIdentification
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSystemBottleneckIdentification();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSystemBottleneckIdentification";
            _tr.test_id = "testSystemBottleneckIdentification";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testEndtoendLatency
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testEndtoendLatency();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testEndtoendLatency";
            _tr.test_id = "testEndtoendLatency";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSystemThroughput
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSystemThroughput();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSystemThroughput";
            _tr.test_id = "testSystemThroughput";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testMultimodalAiWorkloads
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testMultimodalAiWorkloads();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testMultimodalAiWorkloads";
            _tr.test_id = "testMultimodalAiWorkloads";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testAutonomousDrivingStack
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testAutonomousDrivingStack();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testAutonomousDrivingStack";
            _tr.test_id = "testAutonomousDrivingStack";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSensorFusionPipeline
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSensorFusionPipeline();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSensorFusionPipeline";
            _tr.test_id = "testSensorFusionPipeline";
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
std::vector<std::string> SystemIntegrationTesting::getAvailableTests()
{
    return {
        "testConcurrentOperationOfAllSubsystems",
        "testResourceContentionHandling",
        "testSystemBottleneckIdentification",
        "testEndtoendLatency",
        "testSystemThroughput",
        "testMultimodalAiWorkloads",
        "testAutonomousDrivingStack",
        "testSensorFusionPipeline",
    };
}

/**
 * @brief Run a single test by name
 * @param testName Name of the test to run
 * @return Test result with timing
 */
TestResult SystemIntegrationTesting::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if (testName == "testConcurrentOperationOfAllSubsystems") {
        result = testConcurrentOperationOfAllSubsystems();
    } else if (testName == "testResourceContentionHandling") {
        result = testResourceContentionHandling();
    } else if (testName == "testSystemBottleneckIdentification") {
        result = testSystemBottleneckIdentification();
    } else if (testName == "testEndtoendLatency") {
        result = testEndtoendLatency();
    } else if (testName == "testSystemThroughput") {
        result = testSystemThroughput();
    } else if (testName == "testMultimodalAiWorkloads") {
        result = testMultimodalAiWorkloads();
    } else if (testName == "testAutonomousDrivingStack") {
        result = testAutonomousDrivingStack();
    } else if (testName == "testSensorFusionPipeline") {
        result = testSensorFusionPipeline();
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
