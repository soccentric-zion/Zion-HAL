# HAL Refactor — Phase 3: Types and Proto Changes

## C++ type changes — `src/lib_base.h`

### Includes added
- `<cstdint>` — for `uint64_t`
- `<map>` — for `std::map` in `TestOutput`
- `<variant>` — for `hal::Value`

### New `namespace hal` block added
Defines: `Value`, `Status`, `Severity`, `TestOutput`, `TestResult` per spec (Section 1).

### Global `using` declarations added
```cpp
using hal::TestResult;
using hal::TestOutput;
using hal::Status;
using hal::Severity;
using hal::Value;
```
These preserve backward compatibility for all module headers and implementations — no module `.h` file changes are required.

### Old structs removed
- Old `struct TestOutput { bool passed; std::vector<std::string> data; }` — **deleted**
- Old `struct TestResult { std::string name; TestOutput output; std::chrono::duration<double> duration; }` — **deleted**

---

## Proto changes — `protos/common.proto`

### New import added
- `google/protobuf/duration.proto` — for `google.protobuf.Duration` in `TestResult`

### `TestStatus` enum updated
- Added `option allow_alias = true`
- Added prefixed canonical names: `TEST_STATUS_PASS=1`, `TEST_STATUS_FAIL=2`, ..., `TEST_STATUS_ABORTED=5`
- Added `TEST_STATUS_SKIPPED=7` (new value; 5 was ABORTED, 6 was PARTIAL, 7 is fresh)
- Legacy short aliases (`PASS=1`, `FAIL=2`, etc.) retained as deprecated for envelope consumers
- Numeric values **unchanged** — fully backward compatible

### `TestSeverity` enum added (new)
- Tags 0–4: `UNSPECIFIED`, `INFO`, `WARNING`, `ERROR`, `CRITICAL`

### `TypedValue` message added (new)
- `oneof value { bool=1, int64=2, double=3, string=4 }` — mirrors `hal::Value`

### `TestOutput` message updated
- **Reserved:** tags 1, 2 and names "passed", "data"
- New fields at tags 3–9: `status`, `severity`, `reason`, `metrics`, `monitoring`, `event`, `error_message`

### `TestResult` message added (new name)
- Tags 1–8: `module_id`, `test_id`, `name`, `sequence`, `timestamp`, `duration`, `output`, `subtests`
- Uses `google.protobuf.Timestamp` and `google.protobuf.Duration`

### `TestResultEntry` message deprecated
- Body replaced with `reserved 1, 2, 3; reserved "name", "output", "duration_sec";`
- Kept in proto to avoid unknown-field decode errors on rolling deployments

### `MonitoringSnapshot` updated
- `repeated TestResultEntry test_results = 3` → `repeated TestResult test_results = 3`

---

## Module proto changes — `protos/modules/*.proto`

All 35 module protos updated:
```proto
// OLD:
message Result {
  repeated hal.common.TestResultEntry test_results = 1;
}
// NEW:
message Result {
  reserved 1;
  reserved "test_results";
  repeated hal.common.TestResult results = 2;
}
```
Tag 1 reserved per backward-compatibility rules.

**Files changed (35):**
audio_interfaces, camera_interfaces, camera_module3, clock_and_timing,
communication_interface, cpu, debug_and_development_interfaces,
deep_learning_accelerator, display_interfaces, environmental, ethernet,
ethernet_switch, external_connector, functional_safety, gnss, gpio, gpu, imu,
mcu_interface, pcie, poe_hat, power_management, reliability, security_features,
sense_hat, serializer_deserializer, software_interface, storage, system_integration,
system_memory, thermal_management, usb, video_encode_decode, vision_accelerator,
watchdog_and_reset
