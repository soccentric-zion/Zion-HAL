# HAL Refactor — Phase 1: Discovery

**Date:** 2026-05-14  
**Scope:** Full codebase survey for `TestResult` / `TestOutput` / `passed` / `data` references

---

## 1. Repository layout

| Path | Role |
|---|---|
| `src/lib_base.h` | Canonical type definitions (`TestOutput`, `TestResult`, `TestBase`) |
| `src/lib_base.cpp` | `TestBase` runner implementations |
| `src/test_runner.h` | CLI test-runner template used by every test binary |
| `src/<module>/` | 35 module header + implementation pairs |
| `tests/<module>/main.cpp` | Test binary entry points (trivial wrappers) |
| `protos/common.proto` | Shared proto types mirroring `lib_base.h` structs |
| `protos/modules/*.proto` | Per-module Config + Result messages |
| `protos/server/typed_module_handler.h` | Serializer: HAL types → proto messages |
| `protos/server/hal_service_impl.*` | gRPC service implementation |
| `protos/server/module_registry_all.cpp` | Module registration |
| `protos/client/hal_client.*` | gRPC client wrapper |
| `protos/registry/modules.yaml` | Canonical module ID manifest |

---

## 2. Old type declarations — `src/lib_base.h`

```cpp
struct TestOutput {
    bool passed;
    std::vector<std::string> data;
};

struct TestResult {
    std::string name;
    TestOutput output;
    std::chrono::duration<double> duration;
};
```

Both structs are in the **global namespace**.

---

## 3. Files declaring TestResult / TestOutput

| File | Lines | Notes |
|---|---|---|
| `src/lib_base.h` | 32–50 | **Authoritative definition** |
| `protos/common.proto` | 47–56 | Proto mirrors |
| `protos/server/typed_module_handler.h` | 42–54 | Serializer functions |

---

## 4. Files constructing / returning TestResult or TestOutput (module .cpp)

All 35 module `.cpp` files follow the same pattern:

- Each test method returns `TestOutput` via aggregate init `{passed, data}`, `{true, data}`, or `{false, data}`.
- `runAllTests()` builds `std::vector<TestResult>` via aggregate init `{"testName", output, duration}`.
- `runSingleTest()` dispatches to individual tests and returns `TestResult` via aggregate init.

**Count of return patterns across all module .cpp files:**
- `return {passed, data}` — 274 occurrences
- `return {true, data}` — 161 occurrences
- `return {false, data}` — 62 occurrences
- `results.push_back({...})` in `runAllTests()` — ~35 modules × ~10–20 tests each
- `return {testName, result, duration}` in `runSingleTest()` — 35 occurrences

---

## 5. Test files

All 35 test binaries in `tests/*/main.cpp` use the `runTestApp<ClassName>()` template from `src/test_runner.h`. They do **not** directly touch `TestResult` or `TestOutput` — all rendering is in `test_runner.h`.

---

## 6. Proto files mirroring the types

| File | Old messages | Notes |
|---|---|---|
| `protos/common.proto` | `TestOutput` (`passed=1`, `data=2`), `TestResultEntry` (`name=1`, `output=2`, `duration_sec=3`) | Must be updated |
| `protos/modules/*.proto` (50 files) | `Result { repeated hal.common.TestResultEntry test_results = 1; }` | Must reference new `TestResult` |

---

## 7. Serializer

`protos/server/typed_module_handler.h` — functions `ToProto(const ::TestOutput&)` and `ToProto(const ::TestResult&)` — single canonical serialization point.

---

## 8. External consumers

| File | Consumes | Change needed |
|---|---|---|
| `protos/server/typed_module_handler.h` | `r.output.passed`, `r.output.data` | Yes — serializer + `ComputeStatus()` |
| `src/test_runner.h` | `r.output.passed`, `r.output.data`, `r.duration.count()` | Yes |
| `protos/client/hal_client.*` | Proto envelope only (does not unpack TestResultEntry) | No C++ struct changes |

---

## 9. Module count from modules.yaml

**35 active modules registered:**

audio_interfaces, camera_interfaces, camera_module3, clock_and_timing,
communication_interface, cpu, debug_and_development_interfaces,
deep_learning_accelerator, display_interfaces, environmental, ethernet,
ethernet_switch, external_connector, functional_safety, gnss, gpio, gpu, imu,
mcu_interface, pcie, poe_hat, power_management, reliability, security_features,
sense_hat, serializer_deserializer, software_interface, storage,
system_integration, system_memory, thermal_management, usb, video_encode_decode,
vision_accelerator, watchdog_and_reset

All 35 source directories confirmed present under `src/`.

---

## 10. Source path → module_id mapping (from modules.yaml)

| module_id | source_path |
|---|---|
| audio_interfaces | src/audio_interfaces/audio_interfaces.h |
| camera_interfaces | src/camera_interfaces_lib/camera_interfaces_lib.h |
| camera_module3 | src/camera_module3_lib/camera_module3_lib.h |
| clock_and_timing | src/clock_and_timing_lib/clock_and_timing_lib.h |
| communication_interface | src/communication_interface_lib/communication_interface_lib.h |
| cpu | src/cpu_lib/cpu_lib.h |
| debug_and_development_interfaces | src/debug_and_development_interfaces/debug_and_development_interfaces.h |
| deep_learning_accelerator | src/deep_learning_accelerator_lib/deep_learning_accelerator_lib.h |
| display_interfaces | src/display_interfaces/display_interfaces.h |
| environmental | src/environmental_lib/environmental_lib.h |
| ethernet | src/ethernet_lib/ethernet_lib.h |
| ethernet_switch | src/ethernet_switch_lib/ethernet_switch_lib.h |
| external_connector | src/external_connector_lib/external_connector_lib.h |
| functional_safety | src/functional_safety_lib/functional_safety_lib.h |
| gnss | src/gnss_lib/gnss_lib.h |
| gpio | src/gpio_lib/gpio_lib.h |
| gpu | src/gpu_lib/gpu_lib.h |
| imu | src/imu_lib/imu_lib.h |
| mcu_interface | src/mcu_interface_lib/mcu_interface_lib.h |
| pcie | src/pcie_lib/pcie_lib.h |
| poe_hat | src/poe_hat_lib/poe_hat_lib.h |
| power_management | src/power_management_lib/power_management_lib.h |
| reliability | src/reliability_lib/reliability_lib.h |
| security_features | src/security_features_lib/security_features_lib.h |
| sense_hat | src/sense_hat_lib/sense_hat_lib.h |
| serializer_deserializer | src/serializer_deserializer_lib/serializer_deserializer_lib.h |
| software_interface | src/software_interface_lib/software_interface_lib.h |
| storage | src/storage_lib/storage_lib.h |
| system_integration | src/system_integration_lib/system_integration_lib.h |
| system_memory | src/system_memory_lib/system_memory_lib.h |
| thermal_management | src/thermal_management_lib/thermal_management_lib.h |
| usb | src/usb_lib/usb_lib.h |
| video_encode_decode | src/video_encode_decode_lib/video_encode_decode_lib.h |
| vision_accelerator | src/vision_accelerator_lib/vision_accelerator_lib.h |
| watchdog_and_reset | src/watchdog_and_reset_lib/watchdog_and_reset_lib.h |
