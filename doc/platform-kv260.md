# KV260 Platform Support

Zion-HAL targets the **AMD Kria KV260 Vision AI Starter Kit** (K26 SOM, XCK26
Zynq UltraScale+ MPSoC) as a first-class platform, alongside the Zion custom
carrier.

## Selecting the platform at build time

```bash
cmake -B build -DHAL_PLATFORM=kv260     # default; Kria KV260
cmake -B build -DHAL_PLATFORM=zion      # Zion custom Zynq UltraScale+ carrier
```

`HAL_TARGET_NAME` / `HAL_PLATFORM_TARGET` expand to a printable target string;
platform constants live in `src/platform/platform_kv260.h` (KV260) and
`src/platform/platform_zynq.h` (Zion). The compile-time selection is
complemented by **runtime board detection**:

- `src/platform/board_detect.{h,cpp}` — reads
  `/sys/firmware/devicetree/base/{model,compatible}` and `/proc/cpuinfo` and
  classifies the board (KriaKv260 / KriaKr260 / KriaK26SOM / ZionCustom /
  Zynq7000 / RaspberryPi / Unknown). Every module reports this in its
  diagnostics, so a mismatched image is caught immediately.

## KV260 silicon coverage

| Block | Device | HAL module / path |
|---|---|---|
| APU | Quad Cortex-A53 @ 1.5 GHz | `cpu_lib` |
| RPU | Dual Cortex-R5F, split/lockstep | `rpu_lib` (remoteproc sysfs, firmware, cluster-mode, rpmsg, reserved-memory) |
| PL / FPGA | K26 fabric (~256K logic cells) | `fpga_fabric_lib` (fpga_manager, pl clocks, amba/AXI IP enumeration, xmutil/dfx-mgr, xclbin/DTBO inventory) |
| PS-PL link | AXI-Lite control + DMA | `ps_pl_backend` (contract mock) + `UioAxilBackend` (live `/dev/uioN` mmap) |
| GPU | Mali-400 MP2 @ 667 MHz | `gpu_lib` (DRI render node; Mali constants in platform headers) |
| VCU | H.264/H.265 encode+decode (al5e/al5d) | `video_encode_decode_lib` via V4L2 M2M enumeration |
| DPU | Vitis AI DPU (B512 xclbin) | `deep_learning_accelerator_lib` (dfx-mgr app load, render node probe) |
| Display | DisplayPort 1.2 (zynqmp-dpsub) | `display_interfaces` (DRM card probing) |
| Ethernet | PS GEM (RGMII) + PTP | `ethernet_lib`, `clock_and_timing_lib` (`/dev/ptp0`) |
| USB | 4x USB 3.0 via hub | `usb_lib` |
| CAN | 2x PS CAN 2.0B | `can_bus_lib` (SocketCAN) |
| Storage | 16 GB eMMC (SOM), microSD, QSPI (MTD) | `storage_lib` |
| Watchdog | cadence-wdt `watchdog0` | `watchdog_and_reset_lib` |
| Thermal | PS SYSMON + INA260 (carrier) | `thermal_management_lib`, `environmental_lib` |
| Camera | RPi camera connector (IMX219/IMX547 via PL CSI+DPU pipeline) | `camera_interfaces_lib`, `vision_accelerator_lib` |
| Security | Secure boot, som EEPROM, PMU | `security_features_lib` |

## Deploying

```bash
make docker-image                 # build the ARM64 container once
make deploy-kv260 IP_KV260=192.168.1.60
make run-kv260                    # run every HAL module on the KV260
make run-kv260 test_fpga_fabric   # run one module
```

SSH credentials default to the fleet standard (`HAL_SSH_USER`, `HAL_SSH_PASS`
overridable).

## Behaviour without hardware

Every module **probes** its device paths first and returns `SKIPPED` with a
detailed reason when a node/sysfs path is missing (e.g. running on the x86 CI
host or inside the build container). Destructive operations (remoteproc
start/stop, CAN transmit) additionally require explicit opt-in environment
variables (`HAL_ALLOW_RPU_CONTROL=1`, `HAL_ALLOW_CAN_TX=1`).

## Known verification gaps

- K26 exact PL sizes (logic cells/DSP/BRAM) must be confirmed against the
  installed K26 variant (KV260 vs KR260 use the same SOM).
- INA260 PMBus address (default 0x40) and SOM EEPROM layout need one manual
  probe on the carrier.
- VCU V4L2 node numbering varies with image version; the HAL enumerates
  drivers instead of assuming fixed nodes.