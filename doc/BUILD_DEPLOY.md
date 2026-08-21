# Zion-HAL Build and Deployment

## Purpose and scope
Zion-HAL is the C++17 hardware abstraction library and protobuf/gRPC interface for Zion boards. CMake builds static/shared HAL libraries, protobuf code, tests, and CPack packages; the Makefile adds native, ARM64 Docker, quality, documentation, and target-device workflows.

## 1. Prerequisites
- Linux, CMake >= 3.10, Ninja, Make, GCC/Clang with C++17, Threads, protobuf/gRPC tools and libraries, and the dependencies listed by `docker/Dockerfile`.
- Docker/buildx for ARM64 cross-builds; clang-format, cmake-format, cppcheck, Doxygen, and graphviz for optional quality/docs targets.
- For manual validation, the matching Zion board, kernel drivers, device nodes, permissions, and peripherals.

## 2. Setup
Install the native dependencies or build the ARM64 Docker image described above.

## 3. Build and compile
```bash
make configure
make build
```
Native output is under `build/native/bin` and `build/native/lib`; `configure` invokes CMake with Ninja. ARM64 uses:
```bash
make docker-install   # conditional and may use sudo
make docker-image
make docker-configure
make docker-build
```
The Docker image is `zion`, based on Ubuntu 22.04 ARM64, and uses an ARM64 platform.

## 4. Test and lint
```bash
make test
make docker-test
make format
make cppcheck
make doc
```
Native and Docker `test` run CTest. `format` requires clang-format and optionally cmake-format; `cppcheck` requires cppcheck; `doc` requires Doxygen and graphviz. Missing quality tools cause the Makefile checks to fail with install guidance.

## 5. Packaging and artifacts
```bash
make package
make package-deb
make package-rpm
make package-ipk
make docker-package
make docker-package-deb
make docker-package-rpm
make docker-package-ipk
```
CMake/CPack produces DEB and RPM packages; `package_ipk` repackages the DEB as `.ipk`. Native packages are generated in the native build directory; Docker packages use `build/cross`. `make list` shows available binaries and libraries.

## 6. Deployment, install, and run
```bash
make run
make deploy-a IP_A=<target-ip>
make deploy-b IP_B=<target-ip>
make deploy
make run-a IP_A=<target-ip>
make run-b IP_B=<target-ip>
```
Deployment copies `build/cross/lib` and `build/cross/bin` to `/tmp/acu-test` over SSH/SCP. Override `HAL_SSH_USER`, `HAL_SSH_PASS`, `USER`, `PASS`, `IP_A`, and `IP_B` only at invocation time. These targets require `sshpass`, network access, and compatible target boards; credentials in the Makefile defaults are development placeholders.

## 7. Cleanup
```bash
make clean
make docker-clean
```
`clean` removes `build`; `docker-clean` removes the cross build inside the image. `docker-shell` opens the ARM64 environment.

## 8. Hardware, cloud, and manual verification caveats
The library can compile while every physical adapter remains unusable. Validate device-tree/kernel support, `/dev` and `/sys` paths, accelerators, cameras, storage, GPIO, audio, and permissions on the exact hardware revision. Docker tests use ARM64 emulation and cannot establish timing, electrical, driver, or sensor behavior.
