/**
 * @file platform_kv260.h
 * @brief AMD/Xilinx Kria KV260 Vision AI Starter Kit platform constants (K26 SOM + KV260 carrier)
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC
 *
 * Target hardware:
 *   - SOM:  K26 (XCK26), Zynq UltraScale+ MPSoC EV-class:
 *       APU   Quad-core Arm Cortex-A53 @ 1.5 GHz
 *       RPU   Dual-core Arm Cortex-R5F @ ~600 MHz (split or lockstep)
 *       GPU   Arm Mali-400 MP2
 *       VCU   H.264 / H.265 encode & decode (4K)
 *       PL    FPGA fabric (~256K system logic cells, ZU5EV class)
 *   - DDR:  4 GB 64-bit DDR4 @ 2666 Mbps
 *   - SOM storage: 512 Mb QSPI NOR + 16 GB eMMC
 *   - KV260 carrier: microSD, 4x USB 3.0, GbE (PS GEM), DisplayPort 1.2 (PS DP TX),
 *       HDMI input (PL), RPi camera connector (IMX219/IMX547 modules), Pmod,
 *       user LEDs, INA260 power monitor + SOM EEPROM on I2C.
 *
 * Every path is a *candidate* — the HAL probes at runtime and reports SKIPPED
 * when a node is absent.  Constants follow the Kria Ubuntu / PetaLinux 2022.1+
 * device-tree conventions.
 */

#pragma once

#include <cstdint>

// ---------------------------------------------------------------------------
// APU / SoC envelope
// ---------------------------------------------------------------------------
static constexpr int    KV260_APU_CORES           = 4;
static constexpr double KV260_A53_MAX_FREQ_MHZ    = 1500.0;
static constexpr double KV260_A53_MIN_FREQ_MHZ    = 400.0;
static constexpr double KV260_A53_TEMP_MAX_C      = 95.0;

// ---------------------------------------------------------------------------
// RPU (Cortex-R5F) — dual core, split or lockstep, driven via remoteproc
// ---------------------------------------------------------------------------
static constexpr int    KV260_RPU_CORES           = 2;
static constexpr double KV260_RPU_MAX_FREQ_MHZ    = 600.0;
static constexpr const char* KV260_RPROC_SYS_PATH = "/sys/class/remoteproc";
static constexpr const char* KV260_RPU_FW_DIR     = "/lib/firmware";
static constexpr const char* KV260_RPMSG_CTRL_DEV = "/dev/rpmsg_ctrl0";
static constexpr const char* KV260_RPMSG_BUS_PATH = "/sys/bus/rpmsg";

// ---------------------------------------------------------------------------
// GPU — Arm Mali-400 MP2 (NOT VideoCore/v3d; those were Raspberry Pi paths)
// ---------------------------------------------------------------------------
static constexpr int    KV260_GPU_SHADER_CORES    = 2;            // Mali-400 MP2
static constexpr double KV260_GPU_MAX_FREQ_MHZ    = 667.0;
static constexpr const char* KV260_MALI_AMBA_PATH = "/sys/bus/platform/devices/fd4b0000.gpu";
static constexpr const char* KV260_DRM_CARD0      = "/dev/dri/card0";
static constexpr const char* KV260_DRM_RENDER0    = "/dev/dri/renderD128";

// ---------------------------------------------------------------------------
// FPGA / PL fabric
// ---------------------------------------------------------------------------
static constexpr int         KV260_PL_LOGIC_CELLS_K = 256;         // ~256K system logic cells (XCK26)
static constexpr const char* KV260_FPGA_MGR_PATH   = "/sys/class/fpga_manager/fpga0";
static constexpr const char* KV260_FPGA_FIRMWARE_DIR = "/lib/firmware";
static constexpr const char* KV260_AMBA_BUS_PATH   = "/sys/bus/amba/devices";
static constexpr const char* KV260_PLATFORM_BUS_PATH = "/sys/bus/platform/devices";
static constexpr const char* KV260_FIRMWARE_DIR    = "/lib/firmware/xilinx";
static constexpr const char* KV260_XMUTIL_BIN      = "/usr/bin/xmutil";
static constexpr const char* KV260_DFXMGRD_BIN     = "/usr/bin/dfx-mgr";

// PL clock domains exposed by the zynqmp-firmware clock driver.
// Probed under /sys/kernel/debug/clk/<name>/clk_rate on Kria releases.
static constexpr const char* KV260_PL_CLK_NAMES[]    = {"pl0_ref", "pl1_ref", "pl2_ref", "pl3_ref"};
static constexpr int          KV260_PL_CLK_COUNT     = 4;
static constexpr double       KV260_PL_CLK_MAX_MHZ   = 1500.0;
static constexpr const char*  KV260_CLK_DEBUGFS_ROOT = "/sys/kernel/debug/clk";

// ---------------------------------------------------------------------------
// PS-PL deterministic I/O backend contract (AXI-Lite register map "ZION")
// Same wire contract as the Zion PL core; UIO device nodes optional.
// ---------------------------------------------------------------------------
static constexpr const char* KV260_PSPL_UIO_CTRL_PATH  = "/dev/uio0";
static constexpr const char* KV260_PSPL_UIO_DMA_PATH   = "/dev/uio1";

static constexpr std::uint32_t KV260_PL_AXI_REG_MAGIC     = 0x5A494F4Eu; // "ZION"
static constexpr std::uint32_t KV260_PL_AXI_REG_MAP_BYTES = 0x00001000u;

// ---------------------------------------------------------------------------
// DDR (K26 SOM): 4 GB 64-bit DDR4 @ 2666 Mbps
// ---------------------------------------------------------------------------
static constexpr long long  KV260_MEM_4GB_BYTES   = 4LL * 1024 * 1024 * 1024;
static constexpr double     KV260_MEM_SPEED_MT_S  = 2666.0;
static constexpr double     KV260_MEM_BW_GBS      = 21.3;  // theoretical peak
static constexpr int        KV260_MEM_BUS_BITS    = 64;

// ---------------------------------------------------------------------------
// SOM / carrier storage
// ---------------------------------------------------------------------------
static constexpr const char* KV260_QSPI_MTD_PATH  = "/dev/mtd0";     // 512 Mb QSPI NOR
static constexpr const char* KV260_QSPI_MTD_SYS   = "/sys/class/mtd/mtd0";
static constexpr const char* KV260_EMMC_BLK_PATH  = "/dev/mmcblk0";  // 16 GB eMMC (SOM)
static constexpr const char* KV260_SD_BLK_PATH    = "/dev/mmcblk1";  // microSD (carrier)

// ---------------------------------------------------------------------------
// Peripherals
// ---------------------------------------------------------------------------
static constexpr int         KV260_PCIE_GEN                  = 3;   // PS PCIe Gen3 (unused on KV260 carrier — SOM edge only)
static constexpr const char* KV260_PCIE_RC_PATH              = "/sys/bus/pci/devices/0000:00:00.0";

static constexpr const char* KV260_USB3_PATH                 = "/sys/bus/platform/devices/fe200000.usb0";
static constexpr int         KV260_USB3_PORT_COUNT           = 4;   // carrier: 4x USB 3.0 via hub
static constexpr int         KV260_USB2_PORT_COUNT           = 4;

static constexpr const char* KV260_ETH_IFACE                 = "eth0";   // PS GEM0 + RGMII PHY
static constexpr int         KV260_ETH_SPEED_MBPS            = 1000;
static constexpr const char* KV260_PTP_DEV                   = "/dev/ptp0";   // GEM PTP clock

static constexpr const char* KV260_CAN0_IFACE                = "can0";   // PS CAN 2.0B
static constexpr const char* KV260_CAN1_IFACE                = "can1";

static constexpr const char* KV260_DP_DRM_PATH               = "/dev/dri/card0"; // PS DP TX (zynqmp-dpsub)
static constexpr const char* KV260_DP_SUBDEV_PATH            = "/sys/bus/platform/devices/fd4a0000.display";

static constexpr const char* KV260_CSI_DEVICE0               = "/dev/video0"; // 1st V4L2 node
static constexpr const char* KV260_MEDIA_CTL0                = "/dev/media0";
static constexpr const char* KV260_RPI_CAM_I2C               = "/dev/i2c-";   // RPi camera header I2C bus (probe -1)

static constexpr const char* KV260_DEBUG_UART                = "/dev/ttyPS0";
static constexpr const char* KV260_UART1                     = "/dev/ttyPS1";

static constexpr const char* KV260_GPIOCHIP_PATH             = "/dev/gpiochip0"; // zynqmp-gpio controller first chip
static constexpr int         KV260_GPIO_PIN_COUNT            = 174;  // zynqmp-gpio 174 banked pins

// VCU video codec (hardware H.264/H.265 encoder "al5e" / decoder "al5d")
static constexpr const char* KV260_VCU_V4L2_DRIVERS[]        = {"al5e", "al5d", "zynqmp-vcu"};
static constexpr int         KV260_VCU_MAX_WIDTH_PX          = 3840; // 4K capable
static constexpr int         KV260_VCU_MAX_HEIGHT_PX         = 2160;

// DPU — Vitis AI DPU (B512 default on KV260 xclbin app store images)
static constexpr const char* KV260_DPU_DMESG_HINT            = "[drm] Initialized DPU";
static constexpr const char* KV260_XCLBIN_DIR                = "/lib/firmware/xilinx";
static constexpr const char* KV260_DFX_MGR_APP_DIR           = "/opt/xilinx/dfx-mgr/repository";

// ---------------------------------------------------------------------------
// Thermal / power (PS XADC/SYSMON + carrier INA260)
// ---------------------------------------------------------------------------
static constexpr const char* KV260_THERMAL_CPU_ZONE          = "/sys/class/thermal/thermal_zone0";
static constexpr double      KV260_THERMAL_THROTTLE_C        = 85.0;
static constexpr double      KV260_THERMAL_SHUTDOWN_C        = 95.0;
static constexpr const char* KV260_INA260_HWMON_HINT         = "ina260";  // hwmon name substring to probe
static constexpr const char* KV260_INA260_I2C_ADDR           = "0x40";
static constexpr const char* KV260_POWER_SUPPLY_PATH         = "/sys/class/power_supply";

// ---------------------------------------------------------------------------
// Security / misc
// ---------------------------------------------------------------------------
static constexpr const char* KV260_EEPROM_SOM_HINT           = "at24";    // SOM ID EEPROM driver
static constexpr const char* KV260_WATCHDOG_DEV              = "/dev/watchdog0"; // cadence-wdt
static constexpr const char* KV260_FIRMWARE_NODE_PATH        = "/sys/devices/platform/firmware/zynqmp-firmware";
static constexpr const char* KV260_DT_MODEL_PATH             = "/sys/firmware/devicetree/base/model";
static constexpr const char* KV260_DT_COMPAT_PATH            = "/sys/firmware/devicetree/base/compatible";

// KV260 user LEDs / buttons (gpiochip indices resolved at runtime by label)
static constexpr const char* KV260_LED_LABELS[]              = {"led1", "led2", "done", "heartbeat"};
static constexpr int         KV260_USER_LED_COUNT            = 2;