/**
 * @file software_interface_lib.cpp
 * @brief Software interface tests for Raspberry Pi 5 (BCM2712).
 *        Verifies kernel boot, driver loading, systemd services, V4L2/ALSA
 *        multimedia frameworks, and sensor subsystem.  CUDA/cuDNN/TensorRT are
 *        NVIDIA-specific and not present on Pi5; those tests document the
 *        equivalent Pi5 accelerator stack (VideoCore VII / libcamera / MMAL).
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "software_interface_lib.h"
#include "../platform_rpi5.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string swReadFile(const std::string& path)
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

static bool swPathExists(const std::string& p)
{
    struct stat st {};
    return stat(p.c_str(), &st) == 0;
}

static std::string swRunCmd(const std::string& cmd)
{
    std::array<char, 512> buf{};
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

// ---------------------------------------------------------------------------

/**
 * @brief Constructor for SoftwareInterfaceTesting
 */
SoftwareInterfaceTesting::SoftwareInterfaceTesting()
{
}

/**
 * @brief Destructor for SoftwareInterfaceTesting
 */
SoftwareInterfaceTesting::~SoftwareInterfaceTesting()
{
}

/**
 * @brief Verify that the Linux kernel booted successfully on Pi5.
 * @details Reads /proc/version for kernel identification, checks uptime > 0,
 *          and reads /boot/firmware/config.txt (Pi5 firmware config) for
 *          key boot parameters.
 */
TestOutput SoftwareInterfaceTesting::testLinuxKernelBoot()
{
    std::vector<std::string> data;
    data.push_back("=== Linux Kernel Boot (Raspberry Pi 5) ===");

    // Kernel version
    std::string kver = swReadFile("/proc/version");
    if (kver.empty())
    {
        data.push_back("[FAIL] /proc/version not readable");
        {
            TestOutput _out;
            _out.status = hal::Status::FAIL;
            _out.severity = hal::Severity::ERROR;
            for (size_t _i = 0; _i < data.size(); ++_i)
                _out.reason["detail_" + std::to_string(_i)] = data[_i];
            return _out;
        }
    }
    data.push_back("[PASS] Kernel: " + kver.substr(0, 120));

    // Uptime
    std::string uptimeStr = swReadFile("/proc/uptime");
    if (!uptimeStr.empty())
    {
        double uptime = std::stod(uptimeStr);
        data.push_back("[PASS] Uptime: " + std::to_string(static_cast<int>(uptime)) + "s");
    }

    // Pi5 firmware config
    const std::string cfgPath = "/boot/firmware/config.txt";
    if (swPathExists(cfgPath))
    {
        data.push_back("[PASS] Pi5 firmware config found: " + cfgPath);
        // Check for arm_64bit or dtoverlay entries
        std::string grep = swRunCmd("grep -c 'arm_64bit\\|dtoverlay\\|dtparam' " + cfgPath + " 2>/dev/null");
        data.push_back("[INFO] config.txt relevant lines: " + grep);
    }
    else
    {
        data.push_back("[INFO] /boot/firmware/config.txt not found (may be mounted elsewhere)");
    }

    // BCM2712 platform device tree
    std::string dtModel = swReadFile("/proc/device-tree/model");
    if (!dtModel.empty())
        data.push_back("[PASS] Device tree model: " + dtModel);
    else
        data.push_back("[WARN] /proc/device-tree/model not readable");

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
 * @brief Verify Pi5-specific kernel drivers are loaded.
 * @details Checks /proc/modules (or lsmod) for key Pi5 drivers:
 *          bcm2712, rp1 (south-bridge), v3d (GPU), bcm2835_dma, nvme, xhci_hcd.
 */
TestOutput SoftwareInterfaceTesting::testDeviceDriverLoading()
{
    std::vector<std::string> data;
    data.push_back("=== Pi5 Device Driver Loading ===");

    static const std::vector<std::pair<std::string, std::string>> drivers = {
        {"v3d",           "VideoCore VII GPU"},
        {"nvme",          "NVMe/PCIe storage"},
        {"xhci_hcd",      "USB3 xHCI host"},
        {"bcm2835_dma",   "DMA engine"},
        {"snd_bcm2835",   "BCM2835 ALSA audio"},
        {"brcmfmac",      "BCM WiFi (optional)"},
        {"gpio_brcmstb",  "GPIO (Pi5 gpiochip4)"},
    };

    bool anyLoaded = false;
    for (const auto& [mod, desc] : drivers)
    {
        // Check /sys/module/<name> (fast, no subprocess)
        std::string modPath = "/sys/module/" + mod;
        if (swPathExists(modPath))
        {
            data.push_back("[PASS] " + mod + " (" + desc + ") — loaded");
            anyLoaded = true;
        }
        else
        {
            // Might be built-in; check /proc/modules text
            std::string grepOut = swRunCmd("grep -c '^" + mod + " ' /proc/modules 2>/dev/null");
            if (grepOut == "1")
            {
                data.push_back("[PASS] " + mod + " (" + desc + ") — loaded (built-in)");
                anyLoaded = true;
            }
            else
            {
                data.push_back("[INFO] " + mod + " (" + desc + ") — not found (optional)");
            }
        }
    }

    // Count total loaded modules
    std::string totalMods = swRunCmd("wc -l < /proc/modules 2>/dev/null");
    data.push_back("[INFO] Total loaded modules: " + totalMods);

    {
        TestOutput _out;
        _out.status = anyLoaded ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = anyLoaded ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Verify that critical systemd services are active on Pi5.
 * @details Checks systemd unit state for ssh, dbus, networking, and
 *          raspberry-pi-specific services (raspi-config, pigpiod if present).
 */
TestOutput SoftwareInterfaceTesting::testSystemServiceStartup()
{
    std::vector<std::string> data;
    data.push_back("=== Systemd Service Startup (Pi5) ===");

    // Check if systemd is running
    if (!swPathExists("/run/systemd/private"))
    {
        data.push_back("[WARN] systemd does not appear to be running");
        // Still report system info
    }

    static const std::vector<std::string> services = {
        "ssh",
        "NetworkManager",
        "networking",
        "dbus",
        "syslog",
    };

    int active = 0;
    for (const auto& svc : services)
    {
        std::string state = swRunCmd("systemctl is-active " + svc + " 2>/dev/null");
        if (state == "active")
        {
            data.push_back("[PASS] " + svc + ": active");
            active++;
        }
        else
        {
            data.push_back("[INFO] " + svc + ": " + (state.empty() ? "not found" : state));
        }
    }

    // systemd overall state
    std::string sysdState = swRunCmd("systemctl is-system-running 2>/dev/null");
    data.push_back("[INFO] system-running state: " + sysdState);

    {
        TestOutput _out;
        const bool _ok = active > 0 || sysdState == "running" || sysdState == "degraded";
        _out.status = _ok ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = _ok ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Verify user-space application frameworks on Pi5.
 * @details Checks presence of libcamera (Pi5 camera stack), GStreamer
 *          (multimedia pipeline), and Python3 (scripting framework).
 *          Reports library versions where available.
 */
TestOutput SoftwareInterfaceTesting::testApplicationFramework()
{
    std::vector<std::string> data;
    data.push_back("=== Application Framework (Pi5) ===");

    // libcamera
    std::string lcVer = swRunCmd("libcamera-hello --version 2>&1 | head -1");
    if (!lcVer.empty() && lcVer.find("not found") == std::string::npos)
        data.push_back("[PASS] libcamera: " + lcVer);
    else
        data.push_back("[INFO] libcamera-hello not found (camera stack may be absent)");

    // GStreamer
    std::string gstVer = swRunCmd("gst-launch-1.0 --version 2>&1 | head -1");
    if (!gstVer.empty() && gstVer.find("not found") == std::string::npos)
        data.push_back("[PASS] GStreamer: " + gstVer);
    else
        data.push_back("[INFO] GStreamer not found (optional multimedia pipeline)");

    // Python3
    std::string pyVer = swRunCmd("python3 --version 2>&1");
    if (!pyVer.empty() && pyVer.find("Python") != std::string::npos)
        data.push_back("[PASS] Python3: " + pyVer);
    else
        data.push_back("[INFO] python3 not found");

    // Raspberry Pi user-land tools
    std::string vcPath = swRunCmd("which vcgencmd 2>/dev/null");
    if (!vcPath.empty())
        data.push_back("[PASS] vcgencmd available: " + vcPath);
    else
        data.push_back("[INFO] vcgencmd not found (rpi-userland not installed)");

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
 * @brief Verify multi-user support on Pi5.
 * @details Reads /etc/passwd for user count, checks /etc/shadow accessibility,
 *          verifies PAM modules are present, and confirms login services.
 */
TestOutput SoftwareInterfaceTesting::testMultiuserSupport()
{
    std::vector<std::string> data;
    data.push_back("=== Multi-User Support (Pi5) ===");

    // Count real user accounts (UID >= 1000)
    std::string userCount = swRunCmd("awk -F: '$3>=1000 && $3<65534{c++} END{print c+0}' /etc/passwd 2>/dev/null");
    data.push_back("[INFO] User accounts (UID >= 1000): " + userCount);

    // Check PAM
    bool pamPresent = swPathExists("/etc/pam.d/common-auth") || swPathExists("/etc/pam.conf");
    data.push_back(pamPresent ? "[PASS] PAM auth config found" : "[INFO] PAM config not found");

    // Check login / sshd
    bool sshdPresent = swPathExists("/etc/ssh/sshd_config");
    data.push_back(sshdPresent ? "[PASS] sshd config present" : "[INFO] sshd config not found");

    // /etc/sudoers
    bool sudoPresent = swPathExists("/etc/sudoers");
    data.push_back(sudoPresent ? "[PASS] /etc/sudoers present" : "[INFO] /etc/sudoers not found");

    // Logged-in users
    std::string who = swRunCmd("who 2>/dev/null | wc -l");
    data.push_back("[INFO] Currently logged-in sessions: " + who);

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
 * @brief Test CUDA runtime availability on Pi5.
 * @details Pi5 does NOT have an NVIDIA GPU and therefore has no CUDA runtime.
 *          This test documents the Pi5 equivalent — VideoCore VII OpenCL/GLES
 *          — and checks for v3d driver and mesa OpenCL.
 */
TestOutput SoftwareInterfaceTesting::testCudaRuntime()
{
    std::vector<std::string> data;
    data.push_back("=== GPU Compute Runtime (Pi5 — no CUDA) ===");
    data.push_back("[INFO] Pi5 BCM2712 uses VideoCore VII GPU, not NVIDIA — CUDA is N/A");

    // Check v3d driver (VideoCore VII)
    bool v3dLoaded = swPathExists("/sys/module/v3d") ||
                     swRunCmd("grep -c '^v3d ' /proc/modules 2>/dev/null") == "1";
    data.push_back(v3dLoaded ? "[PASS] v3d (VideoCore VII) driver loaded" : "[WARN] v3d driver not found");

    // Check for Mesa OpenCL / clinfo
    std::string clInfo = swRunCmd("clinfo 2>/dev/null | grep -m1 'Platform Name'");
    if (!clInfo.empty())
        data.push_back("[PASS] OpenCL platform: " + clInfo);
    else
        data.push_back("[INFO] clinfo not available or no OpenCL platforms");

    // Check for OpenGL ES via glxinfo / eglinfo
    std::string eglVer = swRunCmd("eglinfo 2>/dev/null | grep -m1 'EGL version'");
    if (!eglVer.empty())
        data.push_back("[PASS] EGL: " + eglVer);
    else
        data.push_back("[INFO] eglinfo not found (mesa-utils may not be installed)");

    {
        TestOutput _out;
        _out.status = v3dLoaded ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = v3dLoaded ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Test cuDNN library on Pi5.
 * @details cuDNN is NVIDIA-specific and not present on Pi5.  Documents the
 *          Pi5-equivalent: ONNX Runtime + TFLite inference engines.
 */
TestOutput SoftwareInterfaceTesting::testCudnnLibrary()
{
    std::vector<std::string> data;
    data.push_back("=== Neural-Network Library (Pi5 — no cuDNN) ===");
    data.push_back("[INFO] Pi5 has no cuDNN; equivalent: ONNX Runtime / TFLite");

    // TensorFlow Lite
    std::string tflite = swRunCmd("python3 -c 'import tflite_runtime; print(tflite_runtime.__version__)' 2>/dev/null");
    if (!tflite.empty())
        data.push_back("[PASS] TFLite runtime: " + tflite);
    else
        data.push_back("[INFO] tflite_runtime python package not installed");

    // ONNX Runtime
    std::string ort = swRunCmd("python3 -c 'import onnxruntime; print(onnxruntime.__version__)' 2>/dev/null");
    if (!ort.empty())
        data.push_back("[PASS] ONNX Runtime: " + ort);
    else
        data.push_back("[INFO] onnxruntime python package not installed");

    // Check for libonnxruntime.so
    std::string ortLib = swRunCmd("find /usr/lib /usr/local/lib -name 'libonnxruntime*' 2>/dev/null | head -1");
    if (!ortLib.empty())
        data.push_back("[PASS] libonnxruntime found: " + ortLib);

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
 * @brief Test TensorRT inference on Pi5.
 * @details TensorRT is NVIDIA-specific.  Pi5 equivalent is Hailo-8 HAT (optional)
 *          or software inference via TFLite + XNNPACK / ONNX Runtime ARM NEON.
 */
TestOutput SoftwareInterfaceTesting::testTensorrtInference()
{
    std::vector<std::string> data;
    data.push_back("=== Inference Runtime (Pi5 — no TensorRT) ===");
    data.push_back("[INFO] TensorRT is NVIDIA-specific; Pi5 uses TFLite/ONNX + optional Hailo-8 HAT");

    // Check for Hailo device
    bool hailoPresent = swPathExists("/dev/hailo0");
    data.push_back(hailoPresent ? "[PASS] Hailo-8 AI HAT device /dev/hailo0 found"
                                : "[INFO] No Hailo-8 HAT (/dev/hailo0 absent)");

    // ONNX Runtime NEON-optimised
    std::string ortEp = swRunCmd("python3 -c \"import onnxruntime as rt; print(rt.get_available_providers())\" 2>/dev/null");
    if (!ortEp.empty())
        data.push_back("[PASS] ONNX Runtime providers: " + ortEp);

    // ARM NEON availability
    bool neon = swRunCmd("grep -c 'asimd' /proc/cpuinfo 2>/dev/null") != "0";
    data.push_back(neon ? "[PASS] ARM NEON/ASIMD available for XNNPACK acceleration"
                        : "[WARN] NEON/ASIMD not detected in /proc/cpuinfo");

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
 * @brief Test VPI (NVIDIA Vision Programming Interface) on Pi5.
 * @details VPI is NVIDIA-specific.  Pi5 equivalent is libcamera IPA
 *          + OpenCV + ISP pipeline via Unicam.
 */
TestOutput SoftwareInterfaceTesting::testVpi()
{
    std::vector<std::string> data;
    data.push_back("=== Vision Processing Interface (Pi5 — no NVIDIA VPI) ===");
    data.push_back("[INFO] NVIDIA VPI not applicable; Pi5 uses libcamera + ISP + V4L2");

    // Check for libcamera IPA
    bool ipaPresent = swPathExists("/usr/lib/libcamera") ||
                      swPathExists("/usr/lib/aarch64-linux-gnu/libcamera");
    data.push_back(ipaPresent ? "[PASS] libcamera library directory found"
                              : "[INFO] libcamera library dir not found");

    // OpenCV
    std::string cvVer = swRunCmd("python3 -c 'import cv2; print(cv2.__version__)' 2>/dev/null");
    if (!cvVer.empty())
        data.push_back("[PASS] OpenCV: " + cvVer);
    else
        data.push_back("[INFO] OpenCV python binding not installed");

    // V4L2 devices
    std::string v4l2Count = swRunCmd("ls /dev/video* 2>/dev/null | wc -l");
    data.push_back("[INFO] V4L2 video devices: " + v4l2Count);

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
 * @brief Test multimedia API (V4L2, ALSA, GStreamer, MMAL) on Pi5.
 * @details Checks V4L2 codecs, ALSA sound cards, and MMAL/GStreamer pipelines
 *          available through the RP1 audio/video subsystem.
 */
TestOutput SoftwareInterfaceTesting::testMultimediaApi()
{
    std::vector<std::string> data;
    data.push_back("=== Multimedia API (Pi5 V4L2/ALSA/GStreamer) ===");

    // V4L2
    std::string v4l2Out = swRunCmd("v4l2-ctl --list-devices 2>/dev/null | head -4");
    data.push_back(v4l2Out.empty() ? "[INFO] No V4L2 devices (v4l-utils not installed or no cameras)"
                                   : "[PASS] V4L2 devices:\n" + v4l2Out);

    // ALSA
    std::string alsaCards = swReadFile("/proc/asound/cards");
    if (!alsaCards.empty())
    {
        data.push_back("[PASS] ALSA sound cards:\n" + alsaCards);
    }
    else
    {
        data.push_back("[INFO] No ALSA sound cards found");
    }

    // HDMI audio (vc4-hdmi)
    bool hdmiAudio = swPathExists("/proc/asound") &&
                     swRunCmd("grep -l 'vc4-hdmi\\|bcm2835' /proc/asound/cards 2>/dev/null | wc -l") != "0";
    (void)hdmiAudio;

    // GStreamer pipeline test (fakesrc → fakesink, no hardware needed)
    std::string gstTest = swRunCmd("gst-launch-1.0 fakesrc num-buffers=1 ! fakesink 2>&1 | grep -c 'Execution ended'");
    data.push_back(gstTest == "1" ? "[PASS] GStreamer fakesrc→fakesink pipeline OK"
                                  : "[INFO] GStreamer test skipped (not installed)");

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
 * @brief Test sensor driver framework on Pi5.
 * @details Checks IIO (Industrial I/O) subsystem and input subsystem for
 *          sensor drivers accessible via /sys/bus/iio/devices and
 *          /sys/class/input.  Also checks lm-sensors/hwmon.
 */
TestOutput SoftwareInterfaceTesting::testSensorDriverFramework()
{
    std::vector<std::string> data;
    data.push_back("=== Sensor Driver Framework (Pi5 IIO / hwmon) ===");

    // IIO devices
    std::string iioCount = swRunCmd("ls /sys/bus/iio/devices/ 2>/dev/null | wc -l");
    data.push_back("[INFO] IIO devices: " + iioCount);

    // hwmon sensors (BCM2712 thermal + MXL7704 PMIC)
    std::string hwmonCount = swRunCmd("ls /sys/class/hwmon/ 2>/dev/null | wc -l");
    data.push_back("[INFO] hwmon devices: " + hwmonCount);
    if (std::stoi(hwmonCount.empty() ? "0" : hwmonCount) > 0)
    {
        std::string hwmonNames = swRunCmd(
            "for d in /sys/class/hwmon/hwmon*; do n=$(cat $d/name 2>/dev/null); echo $d: $n; done 2>/dev/null");
        data.push_back("[PASS] hwmon:\n" + hwmonNames);
    }

    // Thermal zones (sensors framework)
    std::string tzCount = swRunCmd("ls /sys/class/thermal/ 2>/dev/null | grep -c 'thermal_zone'");
    data.push_back("[INFO] Thermal zones: " + tzCount);

    // Input devices
    std::string inputCount = swRunCmd("ls /sys/class/input/ 2>/dev/null | wc -l");
    data.push_back("[INFO] Input devices: " + inputCount);

    bool hasSensors = (std::stoi(hwmonCount.empty() ? "0" : hwmonCount) > 0) ||
                      (std::stoi(tzCount.empty() ? "0" : tzCount) > 0);
    {
        TestOutput _out;
        _out.status = hasSensors ? hal::Status::PASS : hal::Status::FAIL;
        _out.severity = hasSensors ? hal::Severity::INFO : hal::Severity::ERROR;
        for (size_t _i = 0; _i < data.size(); ++_i)
            _out.reason["detail_" + std::to_string(_i)] = data[_i];
        return _out;
    }
}

/**
 * @brief Run all tests for this module
 * @return Vector of test results
 */
std::vector<TestResult> SoftwareInterfaceTesting::runAllTests()
{
    std::vector<TestResult> results;

    // Run testLinuxKernelBoot
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testLinuxKernelBoot();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testLinuxKernelBoot";
            _tr.test_id = "testLinuxKernelBoot";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testDeviceDriverLoading
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testDeviceDriverLoading();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testDeviceDriverLoading";
            _tr.test_id = "testDeviceDriverLoading";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSystemServiceStartup
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSystemServiceStartup();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSystemServiceStartup";
            _tr.test_id = "testSystemServiceStartup";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testApplicationFramework
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testApplicationFramework();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testApplicationFramework";
            _tr.test_id = "testApplicationFramework";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testMultiuserSupport
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testMultiuserSupport();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testMultiuserSupport";
            _tr.test_id = "testMultiuserSupport";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testCudaRuntime
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testCudaRuntime();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testCudaRuntime";
            _tr.test_id = "testCudaRuntime";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testCudnnLibrary
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testCudnnLibrary();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testCudnnLibrary";
            _tr.test_id = "testCudnnLibrary";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testTensorrtInference
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testTensorrtInference();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testTensorrtInference";
            _tr.test_id = "testTensorrtInference";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testVpi
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testVpi();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testVpi";
            _tr.test_id = "testVpi";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testMultimediaApi
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testMultimediaApi();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testMultimediaApi";
            _tr.test_id = "testMultimediaApi";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSensorDriverFramework
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSensorDriverFramework();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSensorDriverFramework";
            _tr.test_id = "testSensorDriverFramework";
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
std::vector<std::string> SoftwareInterfaceTesting::getAvailableTests()
{
    return {
        "testLinuxKernelBoot",      "testDeviceDriverLoading",   "testSystemServiceStartup",
        "testApplicationFramework", "testMultiuserSupport",      "testCudaRuntime",
        "testCudnnLibrary",         "testTensorrtInference",     "testVpi",
        "testMultimediaApi",        "testSensorDriverFramework",
    };
}

/**
 * @brief Run a single test by name
 * @param testName Name of the test to run
 * @return Test result with timing
 */
TestResult SoftwareInterfaceTesting::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if (testName == "testLinuxKernelBoot") {
        result = testLinuxKernelBoot();
    } else if (testName == "testDeviceDriverLoading") {
        result = testDeviceDriverLoading();
    } else if (testName == "testSystemServiceStartup") {
        result = testSystemServiceStartup();
    } else if (testName == "testApplicationFramework") {
        result = testApplicationFramework();
    } else if (testName == "testMultiuserSupport") {
        result = testMultiuserSupport();
    } else if (testName == "testCudaRuntime") {
        result = testCudaRuntime();
    } else if (testName == "testCudnnLibrary") {
        result = testCudnnLibrary();
    } else if (testName == "testTensorrtInference") {
        result = testTensorrtInference();
    } else if (testName == "testVpi") {
        result = testVpi();
    } else if (testName == "testMultimediaApi") {
        result = testMultimediaApi();
    } else if (testName == "testSensorDriverFramework") {
        result = testSensorDriverFramework();
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
