# HAL Refactor — Phase 2: Migration Plan **Order of changes (dependency-first):** 1. `src/lib_base.h` — new canonical C++ types (foundation; everything else depends on this)
2. `protos/common.proto` — new proto types + reserved old fields (must come before regen)
3. `protos/modules/*.proto` — change `TestResultEntry` → `TestResult` in all Result messages
4. Proto regeneration (generated `.pb.h` / `.pb.cc` files)
5. `protos/server/typed_module_handler.h` — new serializer for new types
6. All 35 module `.cpp` files — replace `{passed, data}` aggregate inits with new types
7. `src/test_runner.h` — update all references to old fields
8. `src/lib_base.cpp` — stamp module_id, timestamp, sequence in runner --- ## Change 1: `src/lib_base.h` **Remove:** old `TestOutput`, `TestResult` struct definitions (global namespace). **Add:** `namespace hal { Value, Status, Severity, TestOutput, TestResult }` with exact shape from spec. **Add:** global-scope `using` declarations so all existing code continues to compile:
```cpp
using hal::TestResult;
using hal::TestOutput;
using hal::Status;
using hal::Severity;
using hal::Value;
``` **Rationale:** `using` declarations preserve all existing function signatures and module headers unchanged. --- ## Change 2: `protos/common.proto` ### TestStatus enum
- Keep existing numeric values (1–6) to satisfy backward compatibility.
- Add `option allow_alias = true` and prefixed aliases `TEST_STATUS_PASS = 1`, etc.
- Add `TEST_STATUS_SKIPPED = 7` (new, no old value conflicts).
- Keep `PARTIAL = 6` for existing envelope-level consumers. ### TestSeverity enum (new)
- Fresh enum `TEST_SEVERITY_UNSPECIFIED=0`, `TEST_SEVERITY_INFO=1`, ..., `TEST_SEVERITY_CRITICAL=4`. ### TypedValue message (new)
- `oneof value { bool, int64, double, string }` at tags 1–4. ### TestOutput message (replace body)
- Reserve old tags 1, 2 and names "passed", "data".
- Add new fields at tags 3–9. ### TestResult message (new — replaces TestResultEntry)
- Fresh message `TestResult` with tags 1–8 per spec.
- Old `TestResultEntry` body gets a deprecation comment; all its tag numbers reserved. ### ResultEnvelope
- No field changes (still uses envelope-level `TestStatus`). --- ## Change 3: Module protos (`protos/modules/*.proto`) All 35+ module protos have:
```proto
message Result { repeated hal.common.TestResultEntry test_results = 1;
}
```
Change to:
```proto
message Result { reserved 1; reserved "test_results"; repeated hal.common.TestResult results = 2;
}
``` **Tag 1 is reserved** because the old `repeated hal.common.TestResultEntry test_results = 1` is being retired. Tag 2 is fresh. --- ## Change 4: Proto regeneration Run `protoc` to regenerate C++ bindings. Check that the project configures with CMake. --- ## Change 5: `protos/server/typed_module_handler.h` **Remove:** `ToProto(const ::TestOutput&)` and `ToProto(const ::TestResult&)` using old fields. **Add:**
- `ToProtoTypedValue(const hal::Value&)` — variant → TypedValue oneof
- `ToProtoStatus(hal::Status)` — enum mapping
- `ToProtoSeverity(hal::Severity)` — enum mapping
- `ToProtoTestOutput(const hal::TestOutput&)` — full struct mapping
- `ToProtoTestResult(const hal::TestResult&)` — full struct mapping (recursive for subtests)
- `ComputeStatus(const std::vector<::TestResult>&)` — update to use `output.status`
- Runner stamping: stamp `module_id`, `timestamp`, `sequence` on results after `RunTests` --- ## Change 6: Module `.cpp` files (35 modules) **Applied via Python transformation script** (`hal_refactor/migrate_modules.py`). ### Pattern A — test function returns
```cpp
// OLD
return {passed, data};
// NEW
{ TestOutput _out; _out.status = passed ? hal::Status::PASS : hal::Status::FAIL; _out.severity = passed ? hal::Severity::INFO : hal::Severity::ERROR; for (size_t _i = 0; _i < data.size; ++_i) _out.reason["detail_" + std::to_string(_i)] = data[i]; return _out; }
``` **All unlabeled `data` vector entries map to `reason["detail_N"]`.** All are logged as `[MIGRATION-AMBIGUOUS]` in the audit since original keys are positional indices. ### Pattern B — `runAllTests` push_back
```cpp
// OLD
results.push_back({"testName", result, duration});
// NEW
{ TestResult _tr; _tr.name = "testName"; _tr.test_id = "testName"; _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration); _tr.output = result; results.push_back(_tr); }
``` ### Pattern C — `runSingleTest` return
```cpp
// OLD
return {testName, result, duration};
// NEW
{ TestResult _tr; _tr.name = testName; _tr.test_id = testName; _tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration); _tr.output = result; return _tr; }
``` ### Pattern D — default TestOutput init in runSingleTest
```cpp
// OLD
TestOutput result = {false, {}};
// NEW
TestOutput result; result.status = hal::Status::ERROR; result.severity = hal::Severity::ERROR;
result.error_message = "test not dispatched";
``` --- ## Change 7: `src/test_runner.h` - `r.output.passed` → `r.output.status == hal::Status::PASS`
- `r.output.data` (vector iteration) → `r.output.reason` (map iteration)
- `r.duration.count` → `std::chrono::duration<double>(r.duration).count` (converts ns to seconds)
- `trMakeCriteria` predicates: `t.output.passed` → `t.output.status == hal::Status::PASS` --- ## Change 8: `src/lib_base.cpp` - Runner methods (`runAllTests`, `runSelectedTests`) call virtual methods — no change.
- `runSingleTest` is virtual — no change to base.
- **New:** helper to stamp `timestamp`, `module_id`, `sequence` after `runSelectedTests` returns. Runner stamping moved to `TypedModuleHandler` (Section 5), not `lib_base.cpp`.
