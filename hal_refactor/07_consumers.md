# Phase 7: External Consumers ## Files Surveyed | File | Changes Required | Status |
|---|---|---|
| `protos/client/hal_client.h` | None | No C++ struct access |
| `protos/client/hal_client.cpp` | None | Proto envelope only |
| `protos/server/hal_service_impl.h` | None | RPC dispatch only |
| `protos/server/hal_service_impl.cpp` | None | Returns `grpc::Status::OK` |
| `protos/server/module_registry_all.cpp` | None | Uses `HAL_REGISTER` macro |
| `protos/server/module_registry_all.h` | None | Module registry declarations |
| `protos/server/typed_module_handler.h` | **Done (Phase 4)** | Complete rewrite | ## Duplicate Code Removed Both `protos/common.proto` and `protos/server/typed_module_handler.h` had
a second copy of the old message/class definitions appended after the new
definitions (artifact of Phase 4 replace operation). These were truncated: - `common.proto`: truncated to line 186 (removed lines 187–297 containing old `ExecutionMode`, `TestStatus`, `TestOutput`, `TestResultEntry`, `MonitoringSnapshot`, `CommonConfig`, `MonitoringParams`, `EventCondition`, `ResultEnvelope` duplicates)
- `typed_module_handler.h`: truncated to line 455 (removed lines 456–796 containing old `ToProto`, `ComputeStatus` helpers and the old `TypedModuleHandler` class definition) ## `HAL_REGISTER` Macro ```cpp
#define HAL_REGISTER(reg, id, HalClass, ver, src, PkgNs) \ (reg)->Register((id), \ std::make_unique<TypedModuleHandler< \ HalClass, PkgNs::Config, PkgNs::Result>>((id), (ver), (src)))
``` All 35 registrations use `ResultProto = PkgNs::Result`. `TypedModuleHandler`
now calls `rp.add_results` which is the new field name in `Result` proto.
The old `rp.add_test_results` is gone. ## Wire Compatibility - Module `Result` messages: old tag `1` (test_results) is reserved; new tag `2` (`results`) added. Wire-format safe — old serialized bytes decode to zero `results` entries, not garbage.
- `MonitoringSnapshot`: `test_results` field tag `3` unchanged; message type changed from `TestResultEntry` to `TestResult`. Existing persisted snapshots will partially decode (the new `TestResult` wire format is different).
