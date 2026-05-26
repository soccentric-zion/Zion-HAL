#include "client/hal_client.h"

namespace hal {
namespace client {

HalClient::HalClient(std::shared_ptr<grpc::Channel> channel)
    : stub_(hal::HalService::NewStub(std::move(channel))) {}

grpc::Status HalClient::RunSingleShot(const std::string&               module_id,
                                       const google::protobuf::Message& module_config,
                                       const hal::common::CommonConfig& common,
                                       hal::common::ResultEnvelope*     out) {
    hal::SingleShotRequest req;
    req.set_module_id(module_id);
    *req.mutable_common() = common;
    req.mutable_module_config()->PackFrom(module_config);

    grpc::ClientContext ctx;
    return stub_->RunSingleShot(&ctx, req, out);
}

grpc::Status HalClient::StartLongTerm(const std::string&               module_id,
                                       const google::protobuf::Message& module_config,
                                       const hal::common::CommonConfig& common,
                                       ResultCallback                   on_result) {
    hal::LongTermRequest req;
    req.set_module_id(module_id);
    *req.mutable_common() = common;
    req.mutable_module_config()->PackFrom(module_config);

    grpc::ClientContext ctx;
    auto reader = stub_->StartLongTerm(&ctx, req);

    hal::common::ResultEnvelope env;
    while (reader->Read(&env)) {
        if (on_result) on_result(env);
    }
    return reader->Finish();
}

grpc::Status HalClient::StartMonitoring(const std::string&                   module_id,
                                         const google::protobuf::Message&     module_config,
                                         const hal::common::CommonConfig&     common,
                                         const hal::common::MonitoringParams& monitoring,
                                         ResultCallback                        on_result) {
    hal::MonitoringRequest req;
    req.set_module_id(module_id);
    *req.mutable_common() = common;
    *req.mutable_monitoring() = monitoring;
    req.mutable_module_config()->PackFrom(module_config);

    grpc::ClientContext ctx;
    auto reader = stub_->StartMonitoring(&ctx, req);

    hal::common::ResultEnvelope env;
    while (reader->Read(&env)) {
        if (on_result) on_result(env);
    }
    return reader->Finish();
}

grpc::Status HalClient::StartEventBased(const std::string&                  module_id,
                                         const google::protobuf::Message&    module_config,
                                         const hal::common::CommonConfig&    common,
                                         const hal::common::EventCondition&  condition,
                                         ResultCallback                      on_result) {
    hal::EventBasedRequest req;
    req.set_module_id(module_id);
    *req.mutable_common() = common;
    *req.mutable_condition() = condition;
    req.mutable_module_config()->PackFrom(module_config);

    grpc::ClientContext ctx;
    auto reader = stub_->StartEventBased(&ctx, req);

    hal::common::ResultEnvelope env;
    while (reader->Read(&env)) {
        if (on_result) on_result(env);
    }
    return reader->Finish();
}

grpc::Status HalClient::Stop(const std::string& module_id,
                              const std::string& session_id) {
    hal::StopRequest req;
    req.set_module_id(module_id);
    req.set_session_id(session_id);

    grpc::ClientContext ctx;
    hal::StopResponse resp;
    return stub_->Stop(&ctx, req, &resp);
}

grpc::Status HalClient::Pause(const std::string& module_id,
                               const std::string& session_id) {
    hal::PauseRequest req;
    req.set_module_id(module_id);
    req.set_session_id(session_id);

    grpc::ClientContext ctx;
    hal::PauseResponse resp;
    return stub_->Pause(&ctx, req, &resp);
}

grpc::Status HalClient::Resume(const std::string& module_id,
                                const std::string& session_id) {
    hal::ResumeRequest req;
    req.set_module_id(module_id);
    req.set_session_id(session_id);

    grpc::ClientContext ctx;
    hal::ResumeResponse resp;
    return stub_->Resume(&ctx, req, &resp);
}

grpc::Status HalClient::ListModules(std::vector<hal::ModuleInfo>* modules) {
    hal::ListModulesRequest req;
    grpc::ClientContext ctx;
    hal::ListModulesResponse resp;

    auto status = stub_->ListModules(&ctx, req, &resp);
    if (status.ok())
        modules->assign(resp.modules().begin(), resp.modules().end());
    return status;
}

} // namespace client
} // namespace hal
