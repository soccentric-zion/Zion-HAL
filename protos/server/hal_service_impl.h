#pragma once

// hal_service_impl.h — gRPC service implementation interface for the HAL proto layer.
//
// Three pieces:
//   IModuleHandler    — per-module dispatch interface
//   ModuleRegistry    — thread-safe map from module_id → IModuleHandler
//   HalServiceImpl    — concrete HalService::Service that routes RPCs through the registry

#include "hal_service.grpc.pb.h"
#include "common.pb.h"

#include <grpcpp/grpcpp.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace hal {
namespace server {

// ============================================================
// IModuleHandler
// ============================================================

class IModuleHandler {
  public:
    virtual ~IModuleHandler() = default;

    // Run selected tests once and fill *out.
    virtual grpc::Status RunSingleShot(
        grpc::ServerContext*              ctx,
        const google::protobuf::Any&     module_config,
        const hal::common::CommonConfig& common,
        hal::common::ResultEnvelope*     out) = 0;

    // Run tests in cycles for a fixed duration; write one envelope per cycle.
    virtual grpc::Status StartLongTerm(
        grpc::ServerContext*                              ctx,
        const google::protobuf::Any&                     module_config,
        const hal::common::CommonConfig&                 common,
        grpc::ServerWriter<hal::common::ResultEnvelope>* writer) = 0;

    // Run tests + sample monitor items each cycle; write one envelope per cycle.
    virtual grpc::Status StartMonitoring(
        grpc::ServerContext*                              ctx,
        const google::protobuf::Any&                     module_config,
        const hal::common::CommonConfig&                 common,
        const hal::common::MonitoringParams&             monitoring,
        grpc::ServerWriter<hal::common::ResultEnvelope>* writer) = 0;

    // Poll tests continuously; write an envelope each time the condition fires.
    virtual grpc::Status StartEventBased(
        grpc::ServerContext*                              ctx,
        const google::protobuf::Any&                     module_config,
        const hal::common::CommonConfig&                 common,
        const hal::common::EventCondition&               condition,
        grpc::ServerWriter<hal::common::ResultEnvelope>* writer) = 0;

    // Abort any active background test.
    virtual void Stop() = 0;

    // Pause the active background test (no-op if not running).
    virtual void Pause() = 0;

    // Resume a paused background test (no-op if not paused).
    virtual void Resume() = 0;

    // Return the ModuleInfo proto for ListModules.
    virtual hal::ModuleInfo GetModuleInfo() const = 0;
};

// ============================================================
// ModuleRegistry
// ============================================================

class ModuleRegistry {
  public:
    // Register a handler for module_id (overwrites if already present).
    void Register(const std::string& module_id, std::unique_ptr<IModuleHandler> handler);

    // Find handler by module_id; returns nullptr if not found.
    IModuleHandler* Find(const std::string& module_id) const;

    // Return ModuleInfo for all registered modules, in registration order.
    std::vector<hal::ModuleInfo> ListAll() const;

  private:
    mutable std::mutex                                           mu_;
    std::unordered_map<std::string, std::unique_ptr<IModuleHandler>> handlers_;
    std::vector<std::string>                                     order_; // insertion order
};

// ============================================================
// HalServiceImpl
// ============================================================

class HalServiceImpl final : public hal::HalService::Service {
  public:
    explicit HalServiceImpl(std::unique_ptr<ModuleRegistry> registry);

    grpc::Status RunSingleShot(
        grpc::ServerContext*              ctx,
        const hal::SingleShotRequest*    req,
        hal::common::ResultEnvelope*     resp) override;

    grpc::Status StartLongTerm(
        grpc::ServerContext*                              ctx,
        const hal::LongTermRequest*                      req,
        grpc::ServerWriter<hal::common::ResultEnvelope>* writer) override;

    grpc::Status StartMonitoring(
        grpc::ServerContext*                              ctx,
        const hal::MonitoringRequest*                    req,
        grpc::ServerWriter<hal::common::ResultEnvelope>* writer) override;

    grpc::Status StartEventBased(
        grpc::ServerContext*                              ctx,
        const hal::EventBasedRequest*                    req,
        grpc::ServerWriter<hal::common::ResultEnvelope>* writer) override;

    grpc::Status Stop(
        grpc::ServerContext*   ctx,
        const hal::StopRequest* req,
        hal::StopResponse*      resp) override;

    grpc::Status Pause(
        grpc::ServerContext*    ctx,
        const hal::PauseRequest* req,
        hal::PauseResponse*      resp) override;

    grpc::Status Resume(
        grpc::ServerContext*     ctx,
        const hal::ResumeRequest* req,
        hal::ResumeResponse*      resp) override;

    grpc::Status ListModules(
        grpc::ServerContext*          ctx,
        const hal::ListModulesRequest* req,
        hal::ListModulesResponse*      resp) override;

  private:
    // Returns OK and sets *out on success; NOT_FOUND on unknown module_id.
    grpc::Status Resolve(const std::string& module_id, IModuleHandler** out) const;

    std::unique_ptr<ModuleRegistry> registry_;
};

} // namespace server
} // namespace hal
