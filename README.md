# Zion-HAL

## Project Summary

Device permissions and kernel support must be checked before treating a peripheral failure as a library-only defect.

The source-level adapter inventory is useful for planning coverage, while actual support depends on the matching device tree, kernel interfaces, drivers, libraries, and board wiring. Those dependencies are deliberately visible as verification work.

Zion-HAL is the hardware-abstraction and validation library for the Zion Zynq UltraScale+ MPSoC platform. It is a CMake/Make C++ project with protobuf definitions, Docker-based cross-compilation support, a `src` implementation tree, and unit/integration test directories. Existing documentation describes modular libraries for compute, memory, thermal, power, storage, networking, PCIe, USB, GPIO, camera, audio, video, and accelerator paths, with platform constants centralized in `src/platform_zynq.h`. Device-node and sysfs references include V4L2, DRM/GPU, CSI, UART, Hailo, and Coral examples. These are integration contracts, not evidence that every peripheral is present on every Zion board or that each path is operational. Native builds target Linux hosts or the board; Docker targets an ARM64 Ubuntu build container. The repository also advertises Doxygen, clang-format, and cppcheck workflows. Because this leaf touches real hardware, automated tests can validate interfaces, parsing, and mocks while leaving timing, permissions, kernel drivers, electrical connections, and device-specific behavior for manual checks. The feature table distinguishes implemented repository surfaces from partial hardware integrations and avoids turning the platform description into a verification claim. Use the Makefile to discover exact targets, inspect `tests/`, and use Docker only when the cross-toolchain and target assumptions match the board under test. Zion-Client consumes this layer through the project gRPC/protobuf boundary, but this repository should be validated independently first.

## Feature Status

| # | Name / Description | Implemented? | Test Coverage? | Manual Test |
|---:|---|---|---|---|
| 1 | CMake/Make native build | Implemented | Test targets/config present; build not run | [ ] Manual verification required |
| 2 | ARM64 Docker cross-build | Implemented | Docker path documented; image build unverified | [ ] Manual verification required |
| 3 | Platform constants and device mappings | Implemented | Unit coverage unknown | [ ] Manual verification required |
| 4 | CPU and system-memory validation | Implemented | Tests exist; breadth unknown | [ ] Manual verification required |
| 5 | GPU and deep-learning accelerator adapters | Partial | Hardware-dependent coverage unknown | [ ] Manual verification required |
| 6 | Camera, display, and video V4L2 paths | Partial | Automated coverage not established | [ ] Manual verification required |
| 7 | Audio/I2S, GPIO, USB, PCIe, and Ethernet paths | Partial | Test breadth unknown | [ ] Manual verification required |
| 8 | Storage, thermal, power, and watchdog checks | Partial | Test breadth unknown | [ ] Manual verification required |
| 9 | Protobuf interface definitions | Implemented | Generation/compile coverage implied; not run | [ ] Manual verification required |
| 10 | Doxygen documentation configuration | Implemented | Generation not verified | [ ] Manual verification required |
| 11 | clang-format and cppcheck tooling | Implemented | Tool runs not verified | [ ] Manual verification required |
| 12 | Deployment/run helpers for Zion targets | Partial | Networked deployment unverified | [ ] Manual verification required |

## Tests and Commands

```bash
make build
make run
make help
make format
make cppcheck
```

Inspect `tests/` and the Makefile for exact unit and integration targets.

## Limitations

No target board, kernel driver, accelerator, camera, storage device, or deployment path was manually exercised. Platform-specific documentation contains assumptions that must be checked against the actual Zion hardware revision.

## Detailed Reference

The retained HAL guide below contains the repository's extended platform, build, device-path, and contribution notes.
# Zynq UltraScale+-HAL – Hardware Abstraction Library for Zynq UltraScale+ MPSoC ## Overview Zynq UltraScale+-HAL is a comprehensive hardware abstraction and testing suite targeting the **Zynq UltraScale+ MPSoC** (Zynq PS SoC + PL fabric south-bridge). It provides a modular, extensible architecture for testing and exercising hardware interfaces and system components commonly found in automotive and industrial applications. > **Port history**: Originally developed for Zynq UltraScale+ (Zynq UltraScale+).
> Ported to Zynq UltraScale+ MPSoC (Zynq PS + PL fabric) — all platform-specific paths, constants,
> and device nodes updated for the Zion platform hardware. ### Target Platform | Property | Value |
|---|---|
| SoC | Zynq UltraScale+ MPSoC (quad-core Cortex-A53 @ 1.5 GHz + dual-core Cortex-R5) |
| South-bridge | PL fabric (USB 3, PCIe Gen3 x1, GPIO, I2S, CSI/DSI) |
| GPU | Zynq UltraScale+ (12 shader quad cores, 800 MHz max) |
| Memory | 4 GB / 8 GB LPDDR4X-4267 |
| AI Accelerator | Hailo-8 HAT+ (PCIe, ~26 INT8 TOPS) or Coral USB Edge TPU (~4 TOPS) |
| OS | Zynq OS (64-bit) / Ubuntu 24.04 ARM64 | ### Key Features - **Modular Library Architecture**: Separate libraries for CPU, GPU, DLA (Hailo-8/Coral), video codec (V4L2 M2M), camera (Unicam/PL fabric), audio (PL fabric I2S), PCIe, USB (xhci-hcd), GPIO (gpiochip4), storage, Ethernet, and more.
- **Zynq Platform Header**: Centralised constants in `src/platform_zynq.h` (device paths, thermal thresholds, memory bandwidth).
- **Cross-compilation via Docker**: ARM64 Ubuntu 22.04 container (`zion`).
- **Comprehensive Testing**: Unit tests, integration tests, and deployment workflows.
- **Documentation Generation**: Integrated Doxygen support.
- **Code Quality Tools**: clang-format and cppcheck. ## Prerequisites ### System Requirements - **Operating System**: Linux (Ubuntu 20.04+ recommended for host)
- **Compiler**: GCC 11+ or Clang 14+
- **Build Tools**: - CMake 3.10+ - Ninja build system - Make - Git
- Docker installed and running (for cross-compilation) ## Build Process The project supports two primary build methods: 1. **Native Compilation**: Build directly on a Zynq UltraScale+ MPSoC running 64-bit OS.
2. **Docker-based Cross-Compilation**: Build for ARM64 on any Linux host using the `zion` container. ### Native Compilation (on Zion / Zynq UltraScale+ MPSoC) #### Install Dependencies
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build git \ protobuf-compiler libprotobuf-dev libprotoc-dev \ libgrpc++-dev libsqlite3-dev libboost-system-dev \ libboost-date-time-dev doxygen clang-format cppcheck
``` #### Build and Run Tests
```bash
make build
make run
``` ### Docker-based Cross-Compilation The Docker image is based on `ubuntu:22.04` (ARM64) and is named `zion`. #### Build Process 1. **Ensure Docker is Installed** ```bash make docker-install ``` 2. **Build the Docker Image** ```bash make docker-image ``` This creates a Docker image named `zion` based on `ubuntu:22.04` with all necessary ARM64 build dependencies pre-installed. 3. **Build** ```bash make docker-build ``` This command: - Runs the Docker container with the project mounted - Configures the build system with Ninja generator - Builds all targets - Outputs binaries in the `build/cross/` directory 4. **Deploy and Run on Zion Target** ```bash # For Zion Primary (192.168.1.3) make run-a # For Zion Secondary (192.168.1.13) make run-b # Specify custom IP address make run-b IP_B=192.168.200.2
``` ## Platform-Specific Notes ### AI Inference (DLA)
- **Hailo-8** (PCIe HAT+): detected at `/dev/hailo0`, kernel module `hailo_pci` (vendor `0x1e60`)
- **Coral USB Edge TPU**: detected via USB vendor `18d1` at `/dev/apex_0` ### GPU (Zynq UltraScale+)
- devfreq sysfs: `/sys/class/devfreq/fde00000.gpu/`
- DRM: `/dev/dri/card0`, `/dev/dri/renderD128`
- Module: `v3d` ### Video Codec
- H.264 encode/decode: `zynq-codec` at `/dev/video10` (encoder), `/dev/video11` (decoder)
- H.265/HEVC decode: `rpivid` at `/dev/video18` or `/dev/video19` ### Camera (CSI)
- Unicam CSI-2 controller: `/sys/bus/platform/devices/1f00110000.csi`
- V4L2 device: `/dev/video0`, media controller: `/dev/media0` ### Debug UART
- `/dev/ttyPS0` (115200 8N1 by default on Zynq UltraScale+ MPSoC) ## Make Targets For the full list of supported commands, run: ```bash
make help
``` ## Contributing 1. Follow the established code formatting (use `make format`)
2. Run static analysis before submitting (`make cppcheck`)
3. Add tests for new functionality
4. Update documentation as needed
