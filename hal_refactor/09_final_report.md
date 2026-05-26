# Phase 9: Final Report — HAL Result-Type Refactor

## Summary

All 9 phases of the HAL result-type refactor have been completed successfully.
The full codebase compiles cleanly with zero errors (333/333 targets).

## Objective

Replace the old aggregate `TestOutput {bool passed; vector<string> data;}` and
`TestResult {string name; TestOutput output; chrono::duration<double> duration;}`
types with the new canonical types in `namespace hal`:

```cpp
namespace hal {
  using Value = std::variant<bool, int64_t, double, std::string>;
  enum class Status { PASS, FAIL, ERROR, TIMEOUT, SKIPPED, ABORTED };
  enum class Severity { INFO, WARNING, ERROR, CRITICAL };
  struct TestOutput {
    Status status;
    Severity severity;
    std::map<std::string,std::string> reason;
    std::map<std::string,Value> metrics;
    std::map<std::string,Value> monitoring;
    std::map<std::string,Value> event;
    std::string error_message;
  };
  struct TestResult {
    std::string module_id;
    std::string test_id;
    std::string name;
    uint64_t sequence;
    std::chrono::system_clock::time_point timestamp;
    std::chrono::nanoseconds duration;
    TestOutput output;
    std::vector<TestResult> subtests;
  };
}
```

## Files Changed

### Core

| File | Change |
|---|---|
| `src/lib_base.h` | Added new type definitions in `namespace hal`; added global `using` declarations for backward compat |

### Proto files

| File | Change |
|---|---|
| `protos/common.proto` | Added `TypedValue`, `TestResult`, `TestSeverity`, `TestStatus` aliases, updated `TestOutput`, deprecated `TestResultEntry`, updated `MonitoringSnapshot` |
| 35 × `protos/modules/*.proto` | Reserved old `test_results` (tag 1); added new `results` (tag 2, type `TestResult`) |

### Serializer

| File | Change |
|---|---|
| `protos/server/typed_module_handler.h` | Complete rewrite: `ToProtoStatus`, `ToProtoSeverity`, `ToProtoTypedValue`, `ToProtoTestOutput`, `ToProtoTestResult`, `StampResults`; all 4 test mode handlers updated |

### Module implementations (all 35)

| Module | `.cpp` file |
|---|---|
| audio_interfaces | `src/audio_interfaces/audio_interfaces.cpp` |
| camera_interfaces | `src/camera_interfaces_lib/camera_interfaces_lib.cpp` |
| camera_module3 | `src/camera_module3_lib/camera_module3_lib.cpp` |
| clock_and_timing | `src/clock_and_timing_lib/clock_and_timing_lib.cpp` |
| communication_interface | `src/communication_interface_lib/communication_interface_lib.cpp` |
| cpu | `src/cpu_lib/cpu_lib.cpp` |
| debug_and_development_interfaces | `src/debug_and_development_interfaces/debug_and_development_interfaces.cpp` |
| deep_learning_accelerator | `src/deep_learning_accelerator_lib/deep_learning_accelerator_lib.cpp` |
| display_interfaces | `src/display_interfaces/display_interfaces.cpp` |
| environmental | `src/environmental_lib/environmental_lib.cpp` |
| ethernet | `src/ethernet_lib/ethernet_lib.cpp` |
| ethernet_switch | `src/ethernet_switch_lib/ethernet_switch_lib.cpp` |
| external_connector | `src/external_connector_lib/external_connector_lib.cpp` |
| functional_safety | `src/functional_safety_lib/functional_safety_lib.cpp` |
| gnss | `src/gnss_lib/gnss_lib.cpp` |
| gpio | `src/gpio_lib/gpio_lib.cpp` |
| gpu | `src/gpu_lib/gpu_lib.cpp` |
| imu | `src/imu_lib/imu_lib.cpp` |
| mcu_interface | `src/mcu_interface_lib/mcu_interface_lib.cpp` |
| pcie | `src/pcie_lib/pcie_lib.cpp` |
| poe_hat | `src/poe_hat_lib/poe_hat_lib.cpp` |
| power_management | `src/power_management_lib/power_management_lib.cpp` |
| reliability | `src/reliability_lib/reliability_lib.cpp` |
| security_features | `src/security_features_lib/security_features_lib.cpp` |
| sense_hat | `src/sense_hat_lib/sense_hat_lib.cpp` |
| serializer_deserializer | `src/serializer_deserializer_lib/serializer_deserializer_lib.cpp` |
| software_interface | `src/software_interface_lib/software_interface_lib.cpp` |
| storage | `src/storage_lib/storage_lib.cpp` |
| system_integration | `src/system_integration_lib/system_integration_lib.cpp` |
| system_memory | `src/system_memory_lib/system_memory_lib.cpp` |
| thermal_management | `src/thermal_management_lib/thermal_management_lib.cpp` |
| usb | `src/usb_lib/usb_lib.cpp` |
| video_encode_decode | `src/video_encode_decode_lib/video_encode_decode_lib.cpp` |
| vision_accelerator | `src/vision_accelerator_lib/vision_accelerator_lib.cpp` |
| watchdog_and_reset | `src/watchdog_and_reset_lib/watchdog_and_reset_lib.cpp` |

### Runner

| File | Change |
|---|---|
| `src/test_runner.h` | Updated `trPrintTable()` and `trMakeCriteria()` to use new types |

## Transformation Rules Applied

| Old pattern | New pattern | Count |
|---|---|---|
| `return {true, data}` | New `TestOutput` with `Status::PASS` | ~161 |
| `return {false, data}` | New `TestOutput` with `Status::FAIL` | ~62 + 15 inline |
| `return {passed, data}` | New `TestOutput` with ternary status | ~274 |
| `return {<complex>, data}` | New `TestOutput` with `const bool _ok` | ~29 |
| `results.push_back({name, out, dur})` | New `TestResult _tr` block | ~35 |
| `return {testName, out, dur}` | New `TestResult _tr` block | ~35 |
| `TestOutput r = {false, {}}` | Default-init with `Status::ERROR` | ~8 |
| `return {name+"(UNKNOWN)", {false,{}}, dur}` | Error `TestResult` | ~35 |

## Migration Ambiguities

All unlabeled `data` vector entries were positionally indexed:
`data[0]` → `reason["detail_0"]`, `data[1]` → `reason["detail_1"]`, etc.

This preserves all data without loss. 572 total `[MIGRATION-AMBIGUOUS]` entries
logged in `hal_refactor/05_modules/_ambiguous_log.md`.

Future work: replace positional `detail_N` keys with meaningful semantic names
per-module.

## Hard Rules Compliance

| Rule | Status |
|---|---|
| `passed` must not survive in any code path | ✅ Zero occurrences |
| `sequence` and `timestamp` NOT set inside test functions | ✅ Set only in `StampResults()` |
| `module_id` values unchanged from `modules.yaml` | ✅ Passed through `HAL_REGISTER` |
| Old proto tag numbers not reused | ✅ Tag 1 reserved; new field at tag 2 |
| Old field names not reused | ✅ `test_results` reserved; new field is `results` |
