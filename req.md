# HAL Result-Type Refactor — Apply Across HAL Modules and Protos

You are refactoring the **Hardware Abstraction Layer (HAL)** and its companion **proto layer** in an existing C++ project. The HAL has **35+ modules** already implemented and a proto file already used over gRPC/protobuf between the on-target Client and the x86 Server.

The refactor changes the canonical `TestResult` / `TestOutput` types. You must apply the new shape **everywhere** — HAL headers, every module implementation, every test, the proto definitions, generated code regeneration, and any consumer that reads these types.

This is a long-running task. Time is not a constraint. **Correctness, completeness, and consistency are the only constraints.**

---

## 1. The new canonical types

Replace the current `TestResult` and `TestOutput` with exactly this shape. Do not deviate. Do not add fields. Do not omit fields.

```cpp
#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace hal {

/// Typed value used for metrics, monitoring readings, and event payloads.
/// Strings remain available for non-numeric data; numeric/bool data must NOT
/// be stringified — downstream consumers (charting, thresholds) rely on type.
using Value = std::variant<bool, int64_t, double, std::string>;

/// Terminal outcome of a single test execution.
/// PASS    — test executed and the assertion(s) held.
/// FAIL    — test executed and the assertion(s) did not hold.
/// ERROR   — test could not run to completion (hardware missing, exception, etc.).
/// TIMEOUT — test exceeded its allotted duration.
/// SKIPPED — test was not executed (precondition unmet, intentionally skipped).
/// ABORTED — test was cancelled mid-run by an external signal.
enum class Status {
    PASS,
    FAIL,
    ERROR,
    TIMEOUT,
    SKIPPED,
    ABORTED,
};

/// Severity classifies how serious a non-PASS outcome is, or how informational
/// a PASS is. PASS results are typically INFO. FAIL/ERROR results carry WARNING,
/// ERROR, or CRITICAL depending on impact.
enum class Severity {
    INFO,
    WARNING,
    ERROR,
    CRITICAL,
};

/// Output of a single test execution.
///
/// `reason`     — human-readable explanation strings, keyed (e.g. "why" → "...").
///                Always free-form strings.
/// `metrics`    — measured values produced by the test itself (e.g. throughput,
///                latency). Typed via Value.
/// `monitoring` — environmental variables sampled alongside the test, populated
///                only in MONITORING mode (e.g. temp, voltage). Typed via Value.
/// `event`     — payload attached when this result is the trigger of an event in
///                EVENT_BASED mode (e.g. threshold-crossing value, recent samples).
///                Typed via Value. Empty in other modes.
/// `error_message` — populated only when status is ERROR (or, where applicable,
///                TIMEOUT/ABORTED). Empty otherwise.
struct TestOutput {
    Status status;
    Severity severity;
    std::map<std::string, std::string> reason;
    std::map<std::string, Value> metrics;
    std::map<std::string, Value> monitoring;
    std::map<std::string, Value> event;
    std::string error_message;
};

/// Result of a single test execution.
///
/// `module_id`  — stable HAL module identifier (matches modules.yaml).
/// `test_id`    — stable identifier for this test within the module. Never changes
///                across renames. Used as the long-term key in databases.
/// `name`       — human-readable test name. May change over time; not an identity.
/// `sequence`   — monotonic counter assigned by the runner for streaming modes
///                (LONG_TERM, MONITORING, EVENT_BASED). Starts at 0 within a run.
///                For SINGLE_SHOT, sequence is 0.
/// `timestamp`  — wall-clock time the test result was produced (UTC).
/// `duration`   — wall-clock time the test took to execute. Use nanoseconds for
///                precision; consumers convert as needed.
/// `output`     — terminal outcome and associated data.
/// `subtests`   — optional sub-results for hierarchical tests (e.g. per-core CPU
///                stress). Empty when not hierarchical.
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

} // namespace hal
```

This becomes the **single source of truth for HAL test results**. Every module, every test runner, every serializer, and every proto message must conform.

---

## 2. Migration mapping — from old to new

The previous types were:

```cpp
struct TestOutput {
    bool passed;
    std::vector<std::string> data;   // (also seen as std::map<string,string>)
};

struct TestResult {
    std::string name;
    TestOutput output;
    std::chrono::duration<double> duration;
};
```

Apply this mapping mechanically across the codebase:

| Old field | New field | Migration rule |
|---|---|---|
| `output.passed == true` | `output.status = PASS` | `output.severity = INFO` |
| `output.passed == false` | `output.status = FAIL` | `output.severity = ERROR` (unless the module clearly indicates WARNING/CRITICAL) |
| `output.data` (vector or map) | Split: see below | See decomposition rule |
| `result.name` | `result.name` + `result.test_id` | `test_id` = same value as `name` for now (stable string), unless the module already has a distinct identifier |
| `result.duration` (`duration<double>` seconds) | `result.duration` (`nanoseconds`) | `std::chrono::duration_cast<std::chrono::nanoseconds>(old_duration)` |
| (missing) | `result.module_id` | Derive from the module the test belongs to (use the value already used in `modules.yaml` / the proto registry) |
| (missing) | `result.sequence` | Wired by the runner, not the test itself. Tests do not set sequence; the runner assigns. In single-shot, runner sets 0. |
| (missing) | `result.timestamp` | The runner stamps this when the test returns. Tests do not set timestamp themselves. |
| (missing) | `result.output.error_message` | Populated when status is ERROR; otherwise empty. |
| (missing) | `result.subtests` | Empty unless the module already produces hierarchical results. |

**Decomposing the old `data` field**, in order of preference:

1. If a key is clearly a measurement (numeric value, e.g. `throughput_mbps`, `latency_ns`, `core0_temp`) → into `metrics` as a typed `Value`. Parse the string into the appropriate `double` / `int64_t` / `bool`. If parsing is ambiguous, leave as `std::string` in `metrics`.
2. If a key is clearly an environmental reading sampled by the monitoring path → into `monitoring`.
3. If a key is clearly the payload of an event trigger → into `event`.
4. If a key explains pass/fail (e.g. `"reason"`, `"detail"`, `"why"`, free-form English) → into `reason` (stays string-typed).
5. If unclear → into `reason` as a string. **Never silently discard.** When in doubt, preserve.

Whenever a key's destination is non-obvious, log a `[MIGRATION-AMBIGUOUS]` line referencing the file, the key, and the chosen destination, and record it in the audit report (Section 9).

---

## 3. Proto changes

The proto layer that mirrors these types must be updated in lockstep.

### 3.1 Locate

Find the proto file(s) that define the result envelope and any module-specific `Result` messages. Likely under `protos/` (e.g. `protos/common.proto`, `protos/modules/<name>.proto`). Read them. Do not assume names.

### 3.2 Update `common.proto`

Update or add the following messages in the package used by the existing common proto (do not change the package). Exact names below; adapt only the package and import paths.

```proto
syntax = "proto3";
package hal.common;

import "google/protobuf/timestamp.proto";
import "google/protobuf/duration.proto";

// Mirrors hal::Status. Keep the numeric values stable forever.
enum TestStatus {
  TEST_STATUS_UNSPECIFIED = 0;
  TEST_STATUS_PASS        = 1;
  TEST_STATUS_FAIL        = 2;
  TEST_STATUS_ERROR       = 3;
  TEST_STATUS_TIMEOUT     = 4;
  TEST_STATUS_SKIPPED     = 5;
  TEST_STATUS_ABORTED     = 6;
}

// Mirrors hal::Severity.
enum TestSeverity {
  TEST_SEVERITY_UNSPECIFIED = 0;
  TEST_SEVERITY_INFO        = 1;
  TEST_SEVERITY_WARNING     = 2;
  TEST_SEVERITY_ERROR       = 3;
  TEST_SEVERITY_CRITICAL    = 4;
}

// Mirrors hal::Value (std::variant<bool, int64_t, double, std::string>).
message TypedValue {
  oneof value {
    bool   bool_value   = 1;
    int64  int_value    = 2;
    double double_value = 3;
    string string_value = 4;
  }
}

// Mirrors hal::TestOutput.
message TestOutput {
  TestStatus   status               = 1;
  TestSeverity severity             = 2;
  map<string, string>     reason     = 3;
  map<string, TypedValue> metrics    = 4;
  map<string, TypedValue> monitoring = 5;
  map<string, TypedValue> event      = 6;
  string                  error_message = 7;
}

// Mirrors hal::TestResult.
message TestResult {
  string                      module_id  = 1;
  string                      test_id    = 2;
  string                      name       = 3;
  uint64                      sequence   = 4;
  google.protobuf.Timestamp   timestamp  = 5;
  google.protobuf.Duration    duration   = 6;
  TestOutput                  output     = 7;
  repeated TestResult         subtests   = 8;
}
```

### 3.3 Backward compatibility rules — non-negotiable

- **Never reuse old field tag numbers.** If old `TestOutput` had `bool passed = 1` and `repeated string data = 2`, mark them `reserved` rather than reassigning. Use new tag numbers (as shown above) for the new fields.
- **Never reuse old field names.** Add `reserved "passed", "data";`.
- If an old `passed` field must be temporarily retained for rolling deployment, retain it under its original tag, mark it deprecated, and have the new `status` field be the source of truth. Document this in the audit.
- The `TestStatus` enum's numeric values must never be changed once set. Even if the C++ `Status` enum reorders, the proto numbers stay.

### 3.4 Module-specific protos

For each module-specific proto under `protos/modules/<module>.proto`:
- If its `Result` message embedded the old `TestResult` or duplicated old fields like `passed` / `data`, refactor it to embed the new `hal.common.TestResult` (or its module-extension fields cleanly separated from the common result).
- If the module proto carries module-specific structured data, that data stays in the module proto and is referenced from `Result`. Generic `metrics` / `monitoring` data moves into the common `TestResult.output` maps.

If a module proto becomes redundant (everything it carried is now in `TestResult.output.metrics`), do **not** delete it — keep it as the module's `Config` carrier and let `Result` be the common one. Document any cases where this applies.

### 3.5 Regenerate

After editing protos:
- Regenerate C++ bindings for HAL/Client/Server.
- Regenerate TypeScript bindings for the Web UI (if part of this repo).
- Verify both builds succeed before continuing.

---

## 4. HAL serializer / deserializer changes

Find the code that converts HAL types (`hal::TestResult`, `hal::TestOutput`) ↔ proto messages. There should be one canonical place per direction. If conversion is scattered, consolidate to a single header (`hal_bridge/ResultSerializer.h` or similar).

- `hal::Status` ↔ `hal.common.TestStatus` — direct enum mapping. Trap for `TEST_STATUS_UNSPECIFIED` on the way in and convert to `ERROR` with a clear `error_message` ("unspecified status received").
- `hal::Severity` ↔ `hal.common.TestSeverity` — same pattern.
- `hal::Value` (std::variant) ↔ `hal.common.TypedValue` (oneof) — match on variant index, set the appropriate oneof field; on parse, switch on `value_case()`. Default case → string with the raw repr, plus a `[SERIALIZER-FALLBACK]` log line.
- `std::chrono::system_clock::time_point` ↔ `google.protobuf.Timestamp` — use seconds + nanos split. Wall clock, UTC.
- `std::chrono::nanoseconds` ↔ `google.protobuf.Duration` — seconds + nanos split.
- `subtests` — recursive. Same serializer reused.

Round-trip property: `serialize(deserialize(x)) == x` for every `TestResult` shape. Add unit tests for this.

---

## 5. Required workflow — execute in phases

Long-running task; no human approval between phases. At each phase, write the named report file under `hal_refactor/`.

### Phase 1 — Discovery
1. Locate the HAL repo and the proto repo. Report paths.
2. Find every `.h` / `.hpp` declaration of `TestResult` and `TestOutput`. List all.
3. Find every `.cc` / `.cpp` that constructs, returns, or consumes `TestResult` / `TestOutput`. List all.
4. Find every test file that asserts on these types. List all.
5. Find the proto files that mirror them. List all.
6. Find the serializer(s) that convert between them. List all.
7. Find every external consumer outside HAL (Client adapters, Server handlers) that reads these types — they must be updated too.
8. List all 35+ modules and confirm each was found. If module count from `modules.yaml` doesn't match what you find in source, stop and report.

Write `hal_refactor/01_discovery.md`.

### Phase 2 — Migration plan
For each file from Phase 1, produce a concrete edit plan: what changes, why, in what order. Group by module. Identify shared code that must change first (the type definitions, then the serializer, then the modules, then the tests, then the consumers).

Write `hal_refactor/02_plan.md`.

### Phase 3 — Type and proto changes
1. Update the type-definition header(s) with the exact types from Section 1.
2. Update `common.proto` (and any module protos) per Section 3.
3. Add `reserved` for retired field tags and names.
4. Regenerate proto bindings.
5. Confirm the project still configures (CMake) even if it doesn't yet compile.

Write `hal_refactor/03_types_and_protos.md` summarizing the diff.

### Phase 4 — Serializer
1. Implement / update the canonical HAL ↔ proto serializer per Section 4.
2. Write round-trip unit tests for every status, severity, and `Value` variant.
3. Run serializer tests until green.

Write `hal_refactor/04_serializer.md`.

### Phase 5 — Per-module migration
For each of the 35+ modules, in order:
1. Update every test in the module to populate the new fields.
2. Apply the `data` decomposition rule from Section 2. Log `[MIGRATION-AMBIGUOUS]` lines for unclear keys.
3. Ensure `module_id` is set consistently — derive from `modules.yaml`.
4. Set `test_id` (initially equal to `name` unless a stable id exists).
5. Leave `sequence` and `timestamp` for the runner to stamp (do not set inside the test).
6. Compile the module. Run its tests. Fix all breakage before moving to the next module.

Write `hal_refactor/05_modules/<module_name>.md` per module, summarizing the migration and any ambiguities.

### Phase 6 — Runner updates
The execution runner (single-shot, long-term, monitoring, event-based) must:
1. Stamp `timestamp` on every result it returns.
2. Assign `sequence` (0 for single-shot, monotonic for streaming modes — per-run, not per-process).
3. Populate `monitoring` field in MONITORING mode by sampling configured env variables alongside test execution.
4. Populate `event` field in EVENT_BASED mode with the threshold key, observed value, configured threshold, and any recent samples relevant to the trigger.
5. Set `error_message` if the test threw an exception or could not be invoked, and set `status = ERROR`.
6. Catch test timeouts and set `status = TIMEOUT` with `error_message` describing the timeout.
7. Catch cancellation and set `status = ABORTED`.

Write `hal_refactor/06_runner.md`.

### Phase 7 — External consumers
1. Update Client adapters that wrap HAL results into proto envelopes. They no longer set `passed` — they pass through the populated `TestResult` to the new common proto.
2. Update Server handlers that decode results. Add handling for new statuses (TIMEOUT, SKIPPED, ABORTED) — surface in DB and in REST.
3. Update Server DB writes to persist `status` (not `passed`), `severity`, and the new typed maps. Schema migration if needed — additive only (new columns), retain old columns long enough to backfill if historical data exists.
4. Update Web UI rendering to display the new statuses with distinct color codes; render `TypedValue` numerics as numbers (chartable) and strings as strings; show `reason`, `metrics`, `monitoring`, `event` in separate sections of the result detail; show `error_message` prominently when set.

Write `hal_refactor/07_consumers.md`.

### Phase 8 — Full verification
1. Build every component clean.
2. Run every test suite. All green required.
3. Run a sample end-to-end: SINGLE_SHOT, LONG_TERM, MONITORING, EVENT_BASED on at least three representative modules.
4. Verify a result with each `Status` value can be produced, serialized, stored, fetched via REST, and displayed in the UI.
5. Verify the proto round-trip in serializer tests for every status and every `Value` variant.
6. Verify that no consumer still references `passed` or `data` (grep). If any remain, fix.

Write `hal_refactor/08_verification.md` with evidence (build logs, test summaries).

### Phase 9 — Final report
Single document summarizing:
- All 35+ modules migrated (table: module, files changed, ambiguities, status).
- All proto changes (table: old field, new field, tag treatment).
- All consumer changes.
- All ambiguities resolved (table: file, key, chosen destination, rationale).
- Test coverage before vs after.

Write `hal_refactor/09_final_report.md`.

---

## 6. Hard rules

- **Do not change the new types' shape.** Section 1 is the spec. If something genuinely cannot be expressed within it, stop and write a blocking entry in the audit — do not silently extend.
- **Do not reuse old proto tag numbers or field names.** Always `reserved`.
- **Do not silently drop data.** Every old key in `data` lands somewhere in the new structure, or is logged as ambiguous.
- **Do not let `passed` survive in any code path after Phase 5.** It is replaced by `status == PASS`.
- **Do not let the runner-set fields (`sequence`, `timestamp`) be set inside tests.** Tests produce content; the runner stamps identity and timing.
- **Do not weaken tests.** If an old test asserted on `passed`, the new test asserts on `status == PASS`. Don't loosen the assertion to make migration easier.
- **Do not change `module_id` values.** Whatever string the module had in `modules.yaml` stays the same string in `result.module_id`.
- **Do not change the proto package or the C++ namespace** during this refactor. Type shape only.
- **Do not delete module-specific protos.** Their `Config` role is preserved; only their `Result` role is folded into `hal.common.TestResult`.
- **Schema-driven everywhere.** No new `switch(module_id)` introduced anywhere as a side effect of this refactor.

---

## 7. Deliverables

At completion of Phase 9:
- All 35+ modules building, passing tests, producing the new `TestResult` shape.
- `common.proto` and module protos updated, reserved tags in place, generated bindings regenerated for all consumers.
- A canonical serializer with round-trip tests.
- Updated Client, Server, and Web UI consumers.
- DB schema additive migration applied if needed.
- All reports under `hal_refactor/01_…` through `hal_refactor/09_final_report.md`.
- A grep across the entire repo for `passed` and `\.data` confirming no orphaned references to the old shape remain in HAL-producing or HAL-consuming code paths.

Begin with Phase 1.