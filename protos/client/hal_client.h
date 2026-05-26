#pragma once

// hal_client.h — synchronous gRPC client wrapper for HalService.
//
// Usage (insecure — development only):
//   auto channel = grpc::CreateChannel("localhost:50051",
//                                      grpc::InsecureChannelCredentials());
//   hal::client::HalClient client(channel);
//
// Usage (TLS — production):
//   auto channel_creds = grpc::SslCredentials(grpc::SslCredentialsOptions{});
//   auto channel = grpc::CreateChannel("hal-server.example.com:50051", channel_creds);
//   hal::client::HalClient client(channel);
//
//   hal::modules::cpu::Config cfg;
//   cfg.add_test_names("testL1Cache");
//   cfg.add_test_names("testL2Cache");
//
//   hal::common::ResultEnvelope result;
//   auto status = client.RunSingleShot("cpu", cfg, {}, &result);
//
//   hal::modules::cpu::Result cpu_result;
//   result.payload().UnpackTo(&cpu_result);

#include "hal_service.grpc.pb.h"
#include "common.pb.h"

#include <grpcpp/grpcpp.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace hal {
namespace client {

// Callback invoked for each streamed ResultEnvelope (LongTerm / Monitoring / Event modes).
using ResultCallback = std::function<void(const hal::common::ResultEnvelope&)>;

// ============================================================
// HalClient
// ============================================================
class HalClient {
  public:
    // Construct from any grpc::Channel (insecure, TLS, etc.).
    explicit HalClient(std::shared_ptr<grpc::Channel> channel);

    // ------------------------------------------------------------------
    // RunSingleShot
    //   Run selected (or all) tests once.
    //   module_config: e.g. hal::modules::cpu::Config{}
    //   common:        timing/duration hints (may be default-constructed)
    //   out:           receives the ResultEnvelope; unpack out->payload() to
    //                  hal::modules::<name>::Result
    // ------------------------------------------------------------------
    grpc::Status RunSingleShot(const std::string&                  module_id,
                                const google::protobuf::Message&    module_config,
                                const hal::common::CommonConfig&    common,
                                hal::common::ResultEnvelope*        out);

    // ------------------------------------------------------------------
    // StartLongTerm
    //   Run tests in cycles for common.duration_ms (or indefinitely if
    //   common.run_forever() is true). on_result is called per cycle.
    // ------------------------------------------------------------------
    grpc::Status StartLongTerm(const std::string&                  module_id,
                                const google::protobuf::Message&    module_config,
                                const hal::common::CommonConfig&    common,
                                ResultCallback                      on_result);

    // ------------------------------------------------------------------
    // StartMonitoring
    //   Like StartLongTerm but also samples monitor items each cycle.
    //   Unpack envelope.payload() to hal::common::MonitoringSnapshot.
    // ------------------------------------------------------------------
    grpc::Status StartMonitoring(const std::string&                  module_id,
                                  const google::protobuf::Message&    module_config,
                                  const hal::common::CommonConfig&    common,
                                  const hal::common::MonitoringParams& monitoring,
                                  ResultCallback                       on_result);

    // ------------------------------------------------------------------
    // StartEventBased
    //   Poll continuously; on_result is called each time condition fires.
    // ------------------------------------------------------------------
    grpc::Status StartEventBased(const std::string&                  module_id,
                                  const google::protobuf::Message&    module_config,
                                  const hal::common::CommonConfig&    common,
                                  const hal::common::EventCondition&  condition,
                                  ResultCallback                      on_result);

    // ------------------------------------------------------------------
    // Stop
    //   Stop an active session. Pass empty module_id to stop all modules.
    // ------------------------------------------------------------------
    grpc::Status Stop(const std::string& module_id  = {},
                      const std::string& session_id = {});

    // ------------------------------------------------------------------
    // Pause
    //   Pause an active background session. Pass empty module_id to pause all.
    // ------------------------------------------------------------------
    grpc::Status Pause(const std::string& module_id  = {},
                       const std::string& session_id = {});

    // ------------------------------------------------------------------
    // Resume
    //   Resume a paused session. Pass empty module_id to resume all.
    // ------------------------------------------------------------------
    grpc::Status Resume(const std::string& module_id  = {},
                        const std::string& session_id = {});

    // ------------------------------------------------------------------
    // ListModules
    //   Return ModuleInfo for every module registered on the server.
    // ------------------------------------------------------------------
    grpc::Status ListModules(std::vector<hal::ModuleInfo>* modules);

  private:
    std::unique_ptr<hal::HalService::Stub> stub_;
};

} // namespace client
} // namespace hal
