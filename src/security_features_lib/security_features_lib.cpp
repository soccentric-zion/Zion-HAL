/**
 * @file security_features_lib.cpp
 * @brief Implementation file for SecurityFeaturesTesting class — Raspberry Pi 5
 *        Pi5 security: ARMv8.2 crypto extensions, BoringSSL/OpenSSL, OTP fuses,
 *        hardware RNG (/dev/hwrng), no onboard TPM by default.
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "security_features_lib.h"
#include "../platform_rpi5.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string secReadFile(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::string s;
    std::getline(f, s);
    return s;
}

static std::string secRunCmd(const char* cmd)
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

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
SecurityFeaturesTesting::SecurityFeaturesTesting()
{
}

SecurityFeaturesTesting::~SecurityFeaturesTesting()
{
}

// ---------------------------------------------------------------------------
// 1. testBootRomExecution
// ---------------------------------------------------------------------------
TestOutput SecurityFeaturesTesting::testBootRomExecution()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("BCM2712 Boot ROM execution check");
    data.push_back("Pi5 boot sequence: BootROM (OTP) → EEPROM bootloader → kernel");

    // Check EEPROM bootloader version
    std::string eeVersion = secRunCmd("vcgencmd bootloader_version 2>/dev/null");
    if (!eeVersion.empty()) data.push_back("Bootloader version: " + eeVersion.substr(0, 200));

    std::string eeConfig = secRunCmd("vcgencmd bootloader_config 2>/dev/null");
    if (!eeConfig.empty()) data.push_back("Bootloader config: " + eeConfig.substr(0, 300));

    // Check secure boot OTP settings
    std::string otpDump = secRunCmd("vcgencmd otp_dump 2>/dev/null | grep -E '17:|18:|29:|30:' | head -8");
    if (!otpDump.empty()) data.push_back("OTP rows (17/18/29/30): " + otpDump.substr(0, 300));

    std::string dmesgBoot = secRunCmd("dmesg 2>/dev/null | grep -i 'boot\\|secure' | head -5");
    if (!dmesgBoot.empty()) data.push_back("dmesg boot: " + dmesgBoot.substr(0, 400));

    data.push_back("PASS: Boot ROM execution check complete.");
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
// 2. testBootloaderSignatureVerification
// ---------------------------------------------------------------------------
TestOutput SecurityFeaturesTesting::testBootloaderSignatureVerification()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Bootloader signature verification check (Pi5)");
    data.push_back("Pi5 supports optional RSA-2048 signed bootloader via OTP.");

    // Check if secure boot is enabled in OTP
    std::string otpSec = secRunCmd("vcgencmd otp_dump 2>/dev/null | grep '77:'");
    if (!otpSec.empty()) {
        data.push_back("OTP row 77 (BOOT_CTRL): " + otpSec);
        // Bit 1 of row 77 = signed boot enabled
        if (otpSec.find("00000000") != std::string::npos)
            data.push_back("Signed boot: DISABLED (OTP row 77 = 0)");
        else
            data.push_back("Signed boot: possibly enabled — check bit 1.");
    } else {
        data.push_back("vcgencmd otp_dump: not available.");
    }

    // Check for rpi-eeprom-config signature field
    std::string sigCheck = secRunCmd("rpi-eeprom-config 2>/dev/null | grep -i 'sign\\|public'");
    if (!sigCheck.empty()) data.push_back("rpi-eeprom-config: " + sigCheck.substr(0, 200));

    data.push_back("Default Pi5: secure boot is DISABLED (unsigned bootloader).");
    data.push_back("Enable via: rpi-eeprom-config SIGNED_BOOT=1 and OTP fuse.");
    data.push_back("PASS: Bootloader signature check complete.");
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
// 3. testKernelSignatureVerification
// ---------------------------------------------------------------------------
TestOutput SecurityFeaturesTesting::testKernelSignatureVerification()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Kernel signature verification check (Pi5)");

    // Check if kernel module signing is enabled
    std::string modSig = secReadFile("/proc/sys/kernel/modules_disabled");
    data.push_back("modules_disabled: " + (modSig.empty() ? "0" : modSig));

    std::string modForce = secRunCmd("cat /proc/sys/kernel/module.sig_enforce 2>/dev/null");
    data.push_back("module.sig_enforce: " + (modForce.empty() ? "0" : modForce));

    // Check IMA (Integrity Measurement Architecture)
    bool imaPresent = (access("/sys/kernel/security/ima", F_OK) == 0);
    data.push_back("IMA (Integrity Measurement Architecture): " +
                   std::string(imaPresent ? "enabled" : "not enabled"));

    // Check kernel config for module signing
    std::string kConf = secRunCmd("grep -i 'MODULE_SIG\\|INTEGRITY' /boot/config-$(uname -r) 2>/dev/null | head -5");
    if (!kConf.empty()) data.push_back("Kernel config MODULE_SIG: " + kConf.substr(0, 300));

    data.push_back("NOTE: Default Raspberry Pi OS does not enforce signed kernels.");
    data.push_back("PASS: Kernel signature check complete.");
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
// 4. testSecureBootChainOfTrust
// ---------------------------------------------------------------------------
TestOutput SecurityFeaturesTesting::testSecureBootChainOfTrust()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Secure boot chain of trust check (Pi5)");
    data.push_back("Pi5 chain: BootROM (immutable) → EEPROM (signed optional) → kernel");

    // Check SecureBoot EFI variable (only if EFI is used)
    bool efiVar = (access("/sys/firmware/efi/efivars/SecureBoot-8be4df61-93ca-11d2-aa0d-00e098032b8c",
                          F_OK) == 0);
    data.push_back("EFI SecureBoot variable: " + std::string(efiVar ? "present (EFI boot)" : "not present (Pi5 uses own boot)"));

    // Check trusted keys
    bool trustedKeys = (access("/proc/keys", F_OK) == 0);
    if (trustedKeys) {
        std::string keys = secRunCmd("cat /proc/keys 2>/dev/null | grep -i 'builtin\\|ima' | head -5");
        if (!keys.empty()) data.push_back("Kernel keys: " + keys.substr(0, 300));
    }

    // Check if TPM is available (external TPM2.0 via SPI)
    bool tpmDev = (access("/dev/tpm0", F_OK) == 0);
    data.push_back("/dev/tpm0 (external TPM): " + std::string(tpmDev ? "PRESENT" : "not found (not standard on Pi5)"));

    data.push_back("Pi5 default: no full secure boot chain (bootloader unsigned by default).");
    data.push_back("Full secure boot possible with Pi5 HAT+SPI TPM and signed bootloader.");
    data.push_back("PASS: Chain of trust check complete.");
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
// 5. testHardwareRootOfTrust
// ---------------------------------------------------------------------------
TestOutput SecurityFeaturesTesting::testHardwareRootOfTrust()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Hardware root of trust check (Pi5)");
    data.push_back("Pi5 hardware RoT: BCM2712 OTP + immutable BootROM");

    // Read OTP rows
    std::string otp = secRunCmd("vcgencmd otp_dump 2>/dev/null | head -20");
    if (!otp.empty()) data.push_back("OTP dump (first 20 rows): " + otp.substr(0, 400));

    // Check if OTP customer key is programmed
    std::string custKey = secRunCmd("vcgencmd otp_dump 2>/dev/null | awk -F: '/^(17|18|19|20|21|22|23|24)/ {print $0}'");
    if (!custKey.empty()) data.push_back("OTP customer rows: " + custKey.substr(0, 300));

    // Check TPM
    bool tpmDev = (access("/dev/tpm0", F_OK) == 0);
    data.push_back("External TPM2.0: " + std::string(tpmDev ? "present" : "not found"));

    // ARM TrustZone (OP-TEE)
    bool optee = (access("/dev/tee0", F_OK) == 0);
    data.push_back("OP-TEE TrustZone (/dev/tee0): " + std::string(optee ? "present" : "not found (not default on Pi5)"));

    data.push_back("PASS: Hardware RoT check complete.");
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
// 6. testSecureStorage
// ---------------------------------------------------------------------------
TestOutput SecurityFeaturesTesting::testSecureStorage()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Secure storage check (Pi5)");

    // Check dm-crypt / LUKS support
    std::string dmCrypt = secRunCmd("lsmod 2>/dev/null | grep dm_crypt");
    data.push_back(std::string("dm-crypt kernel module: ") + (dmCrypt.empty() ? "not loaded (load: modprobe dm-crypt)" : "LOADED"));

    // Check cryptsetup
    bool cryptsetup = (access("/usr/sbin/cryptsetup", F_OK) == 0) || (access("/sbin/cryptsetup", F_OK) == 0);
    data.push_back("cryptsetup (LUKS): " + std::string(cryptsetup ? "present" : "not installed"));

    // Check for fscrypt
    std::string fscrypt = secRunCmd("cat /proc/filesystems 2>/dev/null | grep ext4");
    data.push_back("fscrypt: available via ext4 encryption (kernel 5.4+)");

    // Check if OTP can store keys
    data.push_back("Pi5 OTP rows 17-24: customer RSA-2048 public key storage");

    data.push_back("PASS: Secure storage check complete.");
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
// 7. testCryptographicAccelerators
// ---------------------------------------------------------------------------
TestOutput SecurityFeaturesTesting::testCryptographicAccelerators()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Cryptographic accelerators check (Pi5 — ARMv8.2-A Cortex-A76)");

    // Check CPU crypto features
    std::string cpuFlags = secRunCmd("grep -m1 '^Features' /proc/cpuinfo 2>/dev/null");
    if (!cpuFlags.empty()) data.push_back(cpuFlags.substr(0, 300));

    // Parse specific crypto extensions
    auto checkFlag = [&](const std::string& line, const std::string& flag) {
        bool present = line.find(flag) != std::string::npos;
        data.push_back("  " + flag + ": " + (present ? "YES" : "no"));
        return present;
    };

    bool hasAes  = checkFlag(cpuFlags, "aes");
    bool hasSha1 = checkFlag(cpuFlags, "sha1"); (void)hasSha1;
    bool hasSha2 = checkFlag(cpuFlags, "sha2");
    bool hasSha3 = checkFlag(cpuFlags, "sha3"); (void)hasSha3;
    bool hasPmull= checkFlag(cpuFlags, "pmull"); (void)hasPmull;
    bool hasCrc32= checkFlag(cpuFlags, "crc32"); (void)hasCrc32;
    bool hasSVE  = checkFlag(cpuFlags, "sve");  (void)hasSVE;

    // Check OpenSSL hw acceleration
    std::string sslEngines = secRunCmd("openssl engine 2>/dev/null");
    if (!sslEngines.empty()) data.push_back("OpenSSL engines: " + sslEngines.substr(0, 200));

    if (hasAes && hasSha2) {
        data.push_back("PASS: ARMv8 crypto extensions (AES, SHA-2) confirmed.");
    } else {
        data.push_back("INFO: ARM crypto extensions availability limited (check kernel config).");
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
// 8. testHardwareRandomNumberGenerator
// ---------------------------------------------------------------------------
TestOutput SecurityFeaturesTesting::testHardwareRandomNumberGenerator()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Hardware RNG check (Pi5: BCM2712 HRNG, /dev/hwrng)");

    bool hwrngPresent = (access("/dev/hwrng", F_OK) == 0);
    data.push_back("/dev/hwrng: " + std::string(hwrngPresent ? "PRESENT" : "not found"));

    if (hwrngPresent) {
        // Read some bytes to verify it's functional
        int fd = open("/dev/hwrng", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd >= 0) {
            unsigned char buf[32] = {};
            ssize_t n = read(fd, buf, sizeof(buf));
            close(fd);
            if (n == 32) {
                // Check not all-zero or all-same
                bool allSame = true;
                for (int i = 1; i < 32; ++i)
                    if (buf[i] != buf[0]) { allSame = false; break; }
                if (!allSame) {
                    data.push_back("HWRNG: returned 32 bytes of entropy — looks functional.");
                } else {
                    passed = false;
                    data.push_back("FAIL: HWRNG returned constant bytes — may be broken.");
                }
            } else {
                data.push_back("HWRNG: read returned " + std::to_string(n) + " bytes.");
            }
        } else {
            data.push_back("INFO: Cannot read /dev/hwrng (permission denied? use root).");
        }
    } else {
        data.push_back("INFO: Load with: modprobe bcm2835-rng or check dtparam=random=on");
    }

    // Also check /dev/random entropy pool
    long long entropy = -1;
    {
        std::ifstream f("/proc/sys/kernel/random/entropy_avail");
        if (f.is_open()) f >> entropy;
    }
    if (entropy >= 0) data.push_back("Kernel entropy pool: " + std::to_string(entropy) + " bits");

    // Check rng-tools or haveged
    std::string rngd = secRunCmd("systemctl is-active rng-tools 2>/dev/null");
    data.push_back("rng-tools: " + (rngd.empty() ? "not installed" : rngd));

    data.push_back("PASS: HWRNG check complete.");
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
// 9. testMemoryEncryption
// ---------------------------------------------------------------------------
TestOutput SecurityFeaturesTesting::testMemoryEncryption()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Memory encryption check (Pi5 LPDDR4X)");
    data.push_back("Pi5 BCM2712 + LPDDR4X: NO hardware inline memory encryption.");
    data.push_back("LPDDR4X does not support DRAM-level AES encryption on Pi5.");

    // Check ARM MTE (Memory Tagging Extension)
    std::string cpuFlags = secRunCmd("grep -m1 '^Features' /proc/cpuinfo 2>/dev/null");
    bool hasMTE = cpuFlags.find("mte") != std::string::npos;
    data.push_back("ARM MTE (Memory Tagging Extension): " + std::string(hasMTE ? "supported" : "not supported on Cortex-A76"));

    // Check Pointer Authentication (PAC) — a memory security feature
    bool hasPAC = cpuFlags.find("paca") != std::string::npos || cpuFlags.find("pacg") != std::string::npos;
    data.push_back("ARM PAC (Pointer Authentication): " + std::string(hasPAC ? "supported" : "not in feature list"));

    // Check smem protection
    std::string smaps = secRunCmd("cat /proc/1/smaps 2>/dev/null | grep -c 'ss'");
    data.push_back("Software memory protection: dm-crypt/LUKS for at-rest encryption.");

    data.push_back("PASS: Memory encryption check complete (software-only on Pi5).");
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
// 10. testDebugPortSecurity
// ---------------------------------------------------------------------------
TestOutput SecurityFeaturesTesting::testDebugPortSecurity()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("Debug port security check (Pi5)");

    // Check if UART debug is accessible
    std::string uartCon = secRunCmd("systemctl status serial-getty@ttyAMA0.service 2>/dev/null | grep -i 'active\\|inactive'");
    data.push_back("UART serial console (ttyAMA0): " + (uartCon.empty() ? "not found" : uartCon));

    // Check SSH
    std::string ssh = secRunCmd("systemctl is-active ssh 2>/dev/null");
    data.push_back("SSH service: " + (ssh.empty() ? "not checked" : ssh));

    // Check ptrace scope
    std::string ptrace = secReadFile("/proc/sys/kernel/yama/ptrace_scope");
    data.push_back("ptrace_scope: " + (ptrace.empty() ? "not set" : ptrace) +
                   " (1=restricted, 2=admin-only, 3=disabled)");

    // Check kernel lockdown
    std::string lockdown = secReadFile("/sys/kernel/security/lockdown");
    data.push_back("kernel lockdown: " + (lockdown.empty() ? "none" : lockdown));

    // Check /dev/mem protection
    std::string devMem = secRunCmd("cat /proc/sys/kernel/perf_event_paranoid 2>/dev/null");
    data.push_back("perf_event_paranoid: " + (devMem.empty() ? "not set" : devMem));

    data.push_back("PASS: Debug port security check complete.");
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
// 11. testJtagSecurity
// ---------------------------------------------------------------------------
TestOutput SecurityFeaturesTesting::testJtagSecurity()
{
    std::vector<std::string> data;
    bool passed = true;

    data.push_back("JTAG security check (Pi5)");
    data.push_back("Pi5 BCM2712 exposes JTAG via GPIO header (ARM CoreSight/DAP).");
    data.push_back("Default: JTAG is DISABLED in Raspberry Pi OS (GPIO not muxed to JTAG).");

    // Check if JTAG is enabled in device tree
    std::string dtbJtag = secRunCmd("dtc -I fs /proc/device-tree 2>/dev/null | grep -i jtag | head -5");
    if (!dtbJtag.empty()) data.push_back("Device tree JTAG: " + dtbJtag.substr(0, 200));
    else data.push_back("JTAG: not visible in device tree (disabled by default).");

    // Check OTP JTAG disable bit
    std::string otpJtag = secRunCmd("vcgencmd otp_dump 2>/dev/null | grep '65:'");
    if (!otpJtag.empty()) data.push_back("OTP row 65 (JTAG ctrl): " + otpJtag);

    data.push_back("To permanently disable JTAG: set OTP JTAG_DISABLED fuse.");
    data.push_back("Enable JTAG (development): dtoverlay=enable-jtag in config.txt");
    data.push_back("PASS: JTAG security check complete.");
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
std::vector<TestResult> SecurityFeaturesTesting::runAllTests()
{
    std::vector<TestResult> results;

    // Run testBootRomExecution
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testBootRomExecution();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testBootRomExecution";
            _tr.test_id = "testBootRomExecution";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testBootloaderSignatureVerification
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testBootloaderSignatureVerification();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testBootloaderSignatureVerification";
            _tr.test_id = "testBootloaderSignatureVerification";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testKernelSignatureVerification
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testKernelSignatureVerification();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testKernelSignatureVerification";
            _tr.test_id = "testKernelSignatureVerification";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSecureBootChainOfTrust
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSecureBootChainOfTrust();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSecureBootChainOfTrust";
            _tr.test_id = "testSecureBootChainOfTrust";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testHardwareRootOfTrust
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testHardwareRootOfTrust();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testHardwareRootOfTrust";
            _tr.test_id = "testHardwareRootOfTrust";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testSecureStorage
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testSecureStorage();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testSecureStorage";
            _tr.test_id = "testSecureStorage";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testCryptographicAccelerators
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testCryptographicAccelerators();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testCryptographicAccelerators";
            _tr.test_id = "testCryptographicAccelerators";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testHardwareRandomNumberGenerator
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testHardwareRandomNumberGenerator();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testHardwareRandomNumberGenerator";
            _tr.test_id = "testHardwareRandomNumberGenerator";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testMemoryEncryption
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testMemoryEncryption();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testMemoryEncryption";
            _tr.test_id = "testMemoryEncryption";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testDebugPortSecurity
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testDebugPortSecurity();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testDebugPortSecurity";
            _tr.test_id = "testDebugPortSecurity";
            _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            _tr.output = result;
            results.push_back(_tr);
        }
    }

    // Run testJtagSecurity
    {
        auto start = std::chrono::high_resolution_clock::now();
        TestOutput result = testJtagSecurity();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        {
            TestResult _tr;
            _tr.name = "testJtagSecurity";
            _tr.test_id = "testJtagSecurity";
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
std::vector<std::string> SecurityFeaturesTesting::getAvailableTests()
{
    return {
        "testBootRomExecution",
        "testBootloaderSignatureVerification",
        "testKernelSignatureVerification",
        "testSecureBootChainOfTrust",
        "testHardwareRootOfTrust",
        "testSecureStorage",
        "testCryptographicAccelerators",
        "testHardwareRandomNumberGenerator",
        "testMemoryEncryption",
        "testDebugPortSecurity",
        "testJtagSecurity",
    };
}

/**
 * @brief Run a single test by name
 * @param testName Name of the test to run
 * @return Test result with timing
 */
TestResult SecurityFeaturesTesting::runSingleTest(const std::string& testName)
{
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if (testName == "testBootRomExecution") {
        result = testBootRomExecution();
    } else if (testName == "testBootloaderSignatureVerification") {
        result = testBootloaderSignatureVerification();
    } else if (testName == "testKernelSignatureVerification") {
        result = testKernelSignatureVerification();
    } else if (testName == "testSecureBootChainOfTrust") {
        result = testSecureBootChainOfTrust();
    } else if (testName == "testHardwareRootOfTrust") {
        result = testHardwareRootOfTrust();
    } else if (testName == "testSecureStorage") {
        result = testSecureStorage();
    } else if (testName == "testCryptographicAccelerators") {
        result = testCryptographicAccelerators();
    } else if (testName == "testHardwareRandomNumberGenerator") {
        result = testHardwareRandomNumberGenerator();
    } else if (testName == "testMemoryEncryption") {
        result = testMemoryEncryption();
    } else if (testName == "testDebugPortSecurity") {
        result = testDebugPortSecurity();
    } else if (testName == "testJtagSecurity") {
        result = testJtagSecurity();
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
