# HAL Proto Layer

Protocol Buffer + gRPC definitions for the Acadia HAL Independent Validation & Verification suite.

---

## Directory layout

```
protos/
├── common.proto            # Shared types (ExecutionMode, TestStatus, ResultEnvelope, …)
├── hal_service.proto       # Single stable gRPC service definition (never changes on module add/remove)
├── modules/                # One .proto file per HAL module
│   ├── cpu.proto
│   ├── gpu.proto
│   └── …  (35 modules total)
├── registry/
│   └── modules.yaml        # Machine-readable module manifest
├── CMakeLists.txt          # Builds the hal_protos static library
└── README.md               # This file
```

Generated C++ sources are placed under `<build_dir>/protos/generated/` and are **never** committed to the repository.

---

## Architecture overview

The design ensures that **adding or removing a module requires only two localized changes**:

1. Create `protos/modules/<module_id>.proto`
2. Add one entry to `protos/registry/modules.yaml`

`hal_service.proto` and `common.proto` remain untouched.

### How it works

Every `HalService` RPC carries a `google.protobuf.Any module_config` field for the request and a `google.protobuf.Any payload` field inside the returned `ResultEnvelope`. The server uses the `module_id` string to look up the registered module, unpack the `Any`, and dispatch the call — no hand-written dispatch tables, no changes to the service definition.

### Four execution modes

| proto `ExecutionMode` | C++ `TestMode`  | Description |
|-----------------------|-----------------|-------------|
| `SINGLE_SHOT`         | `ONE_SHOT`      | Run once, return one `ResultEnvelope` |
| `LONG_TERM`           | `LONG_TERM`     | Run at interval for a fixed duration, stream `ResultEnvelope` per cycle |
| `MONITORING`          | `MONITORING`    | Run indefinitely, sample monitor items each cycle, stream per cycle |
| `EVENT_BASED`         | `EVENT`         | Poll continuously, stream only when `EventCondition` fires |

---

## Using the library

```cmake
# In your CMakeLists.txt:
target_link_libraries(my_target PRIVATE hal_protos)
```

```cpp
// In your C++ source:
#include "common.pb.h"
#include "hal_service.grpc.pb.h"
#include "modules/cpu.pb.h"

// Pack a module-specific config into the Any field:
hal::modules::cpu::Config cpu_cfg;
cpu_cfg.add_test_names("testL1Cache");
cpu_cfg.add_test_names("testL2Cache");

hal::SingleShotRequest req;
req.set_module_id("cpu");
req.mutable_module_config()->PackFrom(cpu_cfg);

// Unpack the result:
hal::modules::cpu::Result cpu_result;
envelope.payload().UnpackTo(&cpu_result);
for (const auto& entry : cpu_result.test_results()) {
    std::cout << entry.name() << ": "
              << (entry.output().passed() ? "PASS" : "FAIL") << "\n";
}
```

---

## How to add a new module

1. **Create the proto file**

   ```
   protos/modules/<module_id>.proto
   ```

   Minimum content:

   ```protobuf
   syntax = "proto3";
   // Source: src/<module_dir>/<header>.h  (class <ClassName> : public TestBase)
   package hal.modules.<module_id>;
   import "common.proto";

   message Config {
     repeated string test_names = 1;
   }

   message Result {
     repeated hal.common.TestResultEntry test_results = 1;
   }
   ```

2. **Add one entry to `protos/registry/modules.yaml`**

   ```yaml
   - id: <module_id>
     proto_package: hal.modules.<module_id>
     config_type: hal.modules.<module_id>.Config
     result_type: hal.modules.<module_id>.Result
     version: 1
     status: active
     source_path: src/<module_dir>/<header>.h
   ```

3. **Re-run CMake** — `protos/CMakeLists.txt` globs `modules/*.proto`, so no CMake changes are needed.

4. **Register the module on the server** — add the new module class to the server-side registry (outside the proto layer).

That is all. No changes to `hal_service.proto`, `common.proto`, or any dispatch table.

---

## Field-numbering rules (read before editing any .proto)

- Field numbers **start at 1** and **increase monotonically**. They are part of the binary wire format and cannot be changed once a file has been used in production.
- **Never renumber** an existing field, even if you think no data has been serialized yet.
- **Never reuse** a field number after removing a field.
- When removing a field, mark it `reserved` before committing:
  ```protobuf
  reserved 3;
  reserved "old_field_name";
  ```
- When adding fields to an existing message, use the next available number.
- Module IDs in `modules.yaml` are also permanent — never reuse an `id` string.

---

## Ambiguity resolutions

| # | C++ construct | Resolution |
|---|---------------|------------|
| A1 | `MonitorItem::reader` (`std::function<string()>`) | Client sends `repeated string monitor_item_names` in `MonitoringParams`; server maps names to registered reader functions. |
| A2 | `EventCriteria::predicate` (`std::function<bool(...)>`) | Client sends a structured `EventCondition` (test name + expected pass, or freeform expression string); server evaluates it each poll cycle. |
| A3 | `GpioPin`, `GpioDirection`, `GpioPullMode` in `gpio_lib.h` | Omitted — they are internal test-harness state, never part of the public module API. |
| A4 | `chrono::duration<double>` for test duration | Mapped to `double duration_sec` in `TestResultEntry`. |
| A5 | `TestOutput::data` (`vector<string>`) | Mapped to `repeated string data` — free-form diagnostic strings. |
