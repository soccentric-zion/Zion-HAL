# Phase 6: Runner Updates

## File Modified
`src/test_runner.h`

## Changes

### `trMakeCriteria()`
All four criteria predicates updated:

| Criterion | Old | New |
|---|---|---|
| `any-fail` | `!t.output.passed` | `t.output.status != hal::Status::PASS` |
| `all-fail` | `t.output.passed` | `t.output.status == hal::Status::PASS` |
| `all-pass` | `!t.output.passed` | `t.output.status != hal::Status::PASS` |
| `fail-rate-above-N` | `!t.output.passed` | `t.output.status != hal::Status::PASS` |

### `trPrintTable()`

| Item | Old | New |
|---|---|---|
| Pass/fail check | `r.output.passed` | `r.output.status == hal::Status::PASS` |
| Duration display | `r.duration.count()` (seconds as double) | `std::chrono::duration<double>(r.duration).count()` (ns → s) |
| Data output | `r.output.data` (vector, each entry on new line) | `r.output.reason` (map, `[key] value`) + `r.output.error_message` |
| Pass/fail label | `r.output.passed ? "PASS" : "FAIL"` | `ok ? "PASS" : "FAIL"` (from local bool) |

## Reasoning

- `chrono::nanoseconds::count()` returns an integer nanosecond count, not a
  seconds double. The old code was correct because `TestResult::duration` was
  a `chrono::duration<double>` (seconds). After migration, `duration` is
  `chrono::nanoseconds`, so we must cast.
- `reason` is a `map<string,string>`, printed as `[key] value` to preserve
  semantic key names generated during migration.
- `error_message` is printed separately (non-empty only on ERROR/TIMEOUT/ABORTED).
