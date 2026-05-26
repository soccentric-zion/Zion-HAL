# Module Inventory and Zion Backend Mapping

## Module Mapping
| Module | Zion backend mapping |
|---|---|
| audio_interfaces | PS Linux ALSA/I2S + PL deterministic I/O endpoints over AXI-Lite |
| camera_interfaces_lib | PS V4L2 + PL CSI/ISP IP control via AXI-Lite |
| camera_module3_lib | generic camera module path through PL camera IP |
| clock_and_timing_lib | PS clock/thermal telemetry + PL timing IP heartbeat |
| communication_interface_lib | PS I2C/SPI/UART + PL deterministic protocol engines |
| cpu_lib | PS APU core checks, optional R5 hard-RT checks |
| debug_and_development_interfaces | PS debug UART + JTAG telemetry exposure |
| deep_learning_accelerator_lib | optional PL accelerator IP control + DMA |
| display_interfaces | DRM and PL display pipeline endpoints |
| environmental_lib | sensor buses through PS and PL gateways |
| ethernet_lib | PS GEM MAC or PL Ethernet IP validation |
| ethernet_switch_lib | PL switch/bridge IP telemetry checks |
| external_connector_lib | board connector pinmaps mapped to PL GPIO |
| functional_safety_lib | PS/PL watchdog, ECC and fault channels |
| gnss_lib | PS UART and PL timestamp alignment checks |
| gpio_lib | PL GPIO controller through AXI and gpiochip |
| gpu_lib | generic Linux GPU probes; Zion-safe fallback behavior |
| imu_lib | sensor path through PS IIO and PL capture bridge |
| mcu_interface_lib | optional PL soft-MCU or R5 control path |
| pcie_lib | PS/PL PCIe root and endpoint checks |
| poe_hat_lib | PoE controller integration on board-specific management bus |
| power_management_lib | PMIC telemetry via PS bus + PL alarm channels |
| reliability_lib | PS/PL fault counters and uptime semantics |
| security_features_lib | secure boot and key storage checks on Zynq flows |
| sense_hat_lib | generic external sensor board abstraction |
| serializer_deserializer_lib | PL high-speed serial IP checks |
| software_interface_lib | Linux services and kernel capability checks for PetaLinux |
| storage_lib | PS eMMC/NVMe and DMA transfer integrity checks |
| system_integration_lib | end-to-end orchestration across PS/PL modules |
| system_memory_lib | DDR and DMA bandwidth/latency validation |
| thermal_management_lib | PS thermal zones and policy checks |
| usb_lib | PS/PL USB host/device validation |
| video_encode_decode_lib | V4L2 and PL codec pipeline checks |
| vision_accelerator_lib | PL vision IP, DMA and software fallback checks |
| watchdog_and_reset_lib | PS watchdog and PL reset-domain checks |

## Test Mapping
All existing module tests under tests/* remain enabled. Pi-specific assumptions were retargeted to generic Zynq/PL semantics through platform constants and backend text updates.

Additional Zion backend coverage:
- tests/ps_pl_axi/main.cpp

Covers:
- AXI register read/write and range checks.
- DMA success and error paths.
- IP-core handshake command/ack and timeout behavior.
- Fault injection, latching, and clear flow.
