# HAL Refactor — Phase 4: Serializer

## Location
`protos/server/typed_module_handler.h`

## Changes

### Removed functions
- `ToProto(const ::TestOutput&)` — used old `passed` / `data` fields
- `ToProto(const ::TestResult&)` — used old aggregate struct

### Added functions

| Function | Purpose |
|---|---|
| `ToProtoStatus(hal::Status)` | `hal::Status` → `hal.common.TestStatus` enum mapping |
| `ToProtoSeverity(hal::Severity)` | `hal::Severity` → `hal.common.TestSeverity` enum mapping |
| `ToProtoTypedValue(const hal::Value&)` | `std::variant<bool,int64_t,double,string>` → `TypedValue` oneof; falls back to empty string with `[SERIALIZER-FALLBACK]` annotation for unknown index |
| `ToProtoTestOutput(const hal::TestOutput&)` | Full struct mapping; copies all maps |
| `ToProtoTestResult(const hal::TestResult&)` | Full struct mapping; converts `chrono::time_point` and `chrono::nanoseconds` to proto Timestamp/Duration; recursive for `subtests` |
| `StampResults(vector<TestResult>&, module_id, sequence)` | Sets `module_id`, `sequence`, `timestamp` (only if not already set), and `test_id` (from `name` if empty) on all results after HAL runs |

### Updated functions
- `ComputeStatus()` — changed `r.output.passed` → `r.output.status == ::hal::Status::PASS`; `PARTIAL` envelope value still produced for mixed results
- `RunAndStamp()` — new private method replacing old `RunTests()`, calls `StampResults()` after running

### Runner stamping (Phase 6 requirement)
The `StampResults()` helper stamps:
- `module_id` — from `TypedModuleHandler::module_id_`
- `sequence` — 0 for SINGLE_SHOT; monotonic counter per streaming run
- `timestamp` — `std::chrono::system_clock::now()` at result-return time
- `test_id` — copied from `name` if not already set by the module

### EVENT_BASED mode
The event trigger payload is populated in `StartEventBased()` before packing:
- `r.output.event["trigger_test"]` = test name
- `r.output.event["expected_pass"]` = bool
- `r.output.event["observed_pass"]` = bool

### MONITORING mode
Per-cycle `MonitoringSnapshot::monitorData` is copied into each result's
`r.output.monitoring` map (keyed by item name, value as `hal::Value{string}`)
before serialization.

## Round-trip properties

For every `hal::Status` value:
- `serialize(deserialize(TestResult{status=X})).output.status` == `X` (via enum mapping)

For every `hal::Value` variant:
- bool → `bool_value` / `int64_t` → `int_value` / `double` → `double_value` / `string` → `string_value`

Duration round-trip:
- `nanoseconds(N)` → `Duration{seconds=N/1e9, nanos=N%1e9}` → reconstructable to same nanoseconds
