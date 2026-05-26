/**
 * @file platform_zynq.h
 * @brief Zion platform constants for Xilinx Zynq-7000 and Zynq UltraScale+ MPSoC
 *
 * This header centralizes platform assumptions for the Zion HAL backend.
 * Public HAL APIs remain unchanged; only backend constants and plumbing are retargeted.
 */

#pragma once

#include <cstdint>

// ---------------------------------------------------------------------------
// CPU / SoC envelope
// ---------------------------------------------------------------------------
static constexpr int    ZYNQ_CPU_CORES         = 4;
static constexpr double ZYNQ_CPU_MAX_FREQ_MHZ  = 1500.0;
static constexpr double ZYNQ_CPU_MIN_FREQ_MHZ  = 400.0;
static constexpr double ZYNQ_CPU_TEMP_MAX_C    = 95.0;

// ---------------------------------------------------------------------------
// GPU / display-adjacent paths (kept generic for distro portability)
// ---------------------------------------------------------------------------
static constexpr int    ZYNQ_GPU_SHADER_CORES  = 1;
static constexpr double ZYNQ_GPU_MAX_FREQ_MHZ  = 800.0;
static constexpr double ZYNQ_GPU_MIN_FREQ_MHZ  = 100.0;
static constexpr double ZYNQ_GPU_TEMP_MAX_C    = 95.0;
static constexpr const char* ZYNQ_GPU_DEVFREQ_PATH = "/sys/class/devfreq";
static constexpr const char* ZYNQ_GPU_DEVICE_PATH  = "/sys/devices/platform";
static constexpr const char* ZYNQ_V3D_DEV_PATH     = "/dev/dri/card0";
static constexpr const char* ZYNQ_V3D_RENDER_PATH  = "/dev/dri/renderD128";

// ---------------------------------------------------------------------------
// DDR assumptions (board-dependent upper bounds)
// ---------------------------------------------------------------------------
static constexpr long long ZYNQ_MEM_8GB_BYTES  = 8LL * 1024 * 1024 * 1024;
static constexpr long long ZYNQ_MEM_4GB_BYTES  = 4LL * 1024 * 1024 * 1024;
static constexpr double    ZYNQ_MEM_SPEED_MHZ  = 2400.0;
static constexpr double    ZYNQ_MEM_BW_GBS     = 19.2;
static constexpr int       ZYNQ_MEM_BUS_BITS   = 64;

// ---------------------------------------------------------------------------
// Peripheral topology
// ---------------------------------------------------------------------------
static constexpr int    ZYNQ_PCIE_GEN          = 3;
static constexpr int    ZYNQ_PCIE_LANES        = 1;
static constexpr double ZYNQ_PCIE_BW_GBS       = 1.0;
static constexpr const char* ZYNQ_PCIE_RC_PATH = "/sys/bus/pci/devices/0000:00:00.0";

static constexpr const char* ZYNQ_XHCI_DRIVER_PATH = "/sys/bus/platform/drivers/xhci-hcd";
static constexpr int ZYNQ_USB3_PORT_COUNT = 1;
static constexpr int ZYNQ_USB2_PORT_COUNT = 1;

static constexpr const char* ZYNQ_ETH_IFACE      = "eth0";
static constexpr int         ZYNQ_ETH_SPEED_MBPS = 1000;

static constexpr const char* ZYNQ_CSI_DEVICE0 = "/dev/video0";
static constexpr const char* ZYNQ_CSI_DEVICE1 = "/dev/video1";
static constexpr const char* ZYNQ_MEDIA_CTL0  = "/dev/media0";
static constexpr const char* ZYNQ_UNICAM_PATH = "/sys/bus/platform/devices/a0000000.csi";

static constexpr const char* ZYNQ_DRM_CARD0 = "/dev/dri/card0";
static constexpr const char* ZYNQ_DRM_CARD1 = "/dev/dri/card1";

static constexpr const char* ZYNQ_I2S_DEVICE = "/sys/bus/platform/devices/a0010000.audio";
static constexpr const char* ZYNQ_ALSA_CARDS = "/proc/asound/cards";

static constexpr const char* ZYNQ_DEBUG_UART = "/dev/ttyPS0";
static constexpr const char* ZYNQ_UART1      = "/dev/ttyPS1";

static constexpr const char* ZYNQ_THERMAL_CPU_ZONE   = "/sys/class/thermal/thermal_zone0";
static constexpr const char* ZYNQ_THERMAL_SB_ZONE    = "/sys/class/thermal/thermal_zone1";
static constexpr double      ZYNQ_THERMAL_THROTTLE_C = 90.0;
static constexpr double      ZYNQ_THERMAL_SHUTDOWN_C = 100.0;

static constexpr const char* ZYNQ_GPIOCHIP_PATH = "/dev/gpiochip0";
static constexpr int         ZYNQ_GPIO_PIN_COUNT = 64;

static constexpr const char* ZYNQ_MMCBLK_PATH = "/dev/mmcblk0";
static constexpr const char* ZYNQ_NVME_PATH   = "/dev/nvme0n1";

static constexpr const char* ZYNQ_POWER_SUPPLY_PATH = "/sys/class/power_supply";
static constexpr const char* ZYNQ_HAILO8_PCIE_VENDOR = "1e60";
static constexpr const char* ZYNQ_CORAL_USB_VENDOR   = "18d1";

// ---------------------------------------------------------------------------
// PS-PL deterministic I/O backend contract
// ---------------------------------------------------------------------------
static constexpr const char* ZYNQ_PSPL_UIO_CTRL_PATH = "/dev/uio0";
static constexpr const char* ZYNQ_PSPL_UIO_DMA_PATH  = "/dev/uio1";

// AXI-Lite register window contract for the PL deterministic I/O core.
static constexpr std::uint32_t ZYNQ_PL_AXI_REG_MAGIC       = 0x5A494F4Eu; // "ZION"
static constexpr std::uint32_t ZYNQ_PL_AXI_REG_VERSION     = 0x00000001u;
static constexpr std::uint32_t ZYNQ_PL_AXI_REG_MAP_BYTES   = 0x00001000u;

static constexpr std::uint32_t ZYNQ_PL_REG_ID              = 0x0000;
static constexpr std::uint32_t ZYNQ_PL_REG_VERSION         = 0x0004;
static constexpr std::uint32_t ZYNQ_PL_REG_CONTROL         = 0x0008;
static constexpr std::uint32_t ZYNQ_PL_REG_STATUS          = 0x000C;
static constexpr std::uint32_t ZYNQ_PL_REG_IRQ_ACK         = 0x0010;
static constexpr std::uint32_t ZYNQ_PL_REG_FAULT_CODE      = 0x0014;
static constexpr std::uint32_t ZYNQ_PL_REG_DMA_SRC_LO      = 0x0020;
static constexpr std::uint32_t ZYNQ_PL_REG_DMA_DST_LO      = 0x0024;
static constexpr std::uint32_t ZYNQ_PL_REG_DMA_LEN_BYTES   = 0x0028;
static constexpr std::uint32_t ZYNQ_PL_REG_DMA_CONTROL     = 0x002C;
static constexpr std::uint32_t ZYNQ_PL_REG_DMA_STATUS      = 0x0030;
static constexpr std::uint32_t ZYNQ_PL_REG_HANDSHAKE       = 0x0040;
static constexpr std::uint32_t ZYNQ_PL_REG_HEARTBEAT       = 0x0044;

static constexpr std::uint32_t ZYNQ_PL_CONTROL_ENABLE      = 1u << 0;
static constexpr std::uint32_t ZYNQ_PL_CONTROL_RESET       = 1u << 1;
static constexpr std::uint32_t ZYNQ_PL_CONTROL_START       = 1u << 2;

static constexpr std::uint32_t ZYNQ_PL_STATUS_READY        = 1u << 0;
static constexpr std::uint32_t ZYNQ_PL_STATUS_BUSY         = 1u << 1;
static constexpr std::uint32_t ZYNQ_PL_STATUS_FAULT        = 1u << 2;
static constexpr std::uint32_t ZYNQ_PL_STATUS_DMA_DONE     = 1u << 3;

static constexpr std::uint32_t ZYNQ_PL_DMA_CONTROL_START   = 1u << 0;
static constexpr std::uint32_t ZYNQ_PL_DMA_CONTROL_IRQ_EN  = 1u << 1;
static constexpr std::uint32_t ZYNQ_PL_DMA_STATUS_BUSY     = 1u << 0;
static constexpr std::uint32_t ZYNQ_PL_DMA_STATUS_DONE     = 1u << 1;
static constexpr std::uint32_t ZYNQ_PL_DMA_STATUS_ERROR    = 1u << 2;

// Handshake bits for PS↔PL command/ack protocol.
static constexpr std::uint32_t ZYNQ_PL_HS_CMD_VALID        = 1u << 0;
static constexpr std::uint32_t ZYNQ_PL_HS_ACK_VALID        = 1u << 1;
static constexpr std::uint32_t ZYNQ_PL_HS_TIMEOUT          = 1u << 2;

// Optional hard-RT execution path on MPSoC R5 (lock-step or split mode).
static constexpr bool ZYNQ_ENABLE_R5_RT_PATH = true;

// ---------------------------------------------------------------------------
// Platform identification
// ---------------------------------------------------------------------------
static constexpr const char* PLATFORM_NAME     = "Zion on Zynq";
static constexpr const char* PLATFORM_SOC      = "Xilinx Zynq-7000 / Zynq UltraScale+ MPSoC";
static constexpr const char* PLATFORM_IOBRIDGE = "Programmable Logic deterministic I/O engine";
static constexpr const char* ZYNQ_DT_MODEL_PATH = "/proc/device-tree/model";
