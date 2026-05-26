#include "server/hal_service_impl.h"

namespace hal {
namespace server {

// ============================================================
// ModuleRegistry
// ============================================================

void ModuleRegistry::Register(const std::string&              module_id,
                               std::unique_ptr<IModuleHandler> handler) {
    std::lock_guard<std::mutex> lk(mu_);
    if (handlers_.find(module_id) == handlers_.end())
        order_.push_back(module_id);
    handlers_[module_id] = std::move(handler);
}

IModuleHandler* ModuleRegistry::Find(const std::string& module_id) const {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = handlers_.find(module_id);
    return it != handlers_.end() ? it->second.get() : nullptr;
}

std::vector<hal::ModuleInfo> ModuleRegistry::ListAll() const {
    std::lock_guard<std::mutex> lk(mu_);
    std::vector<hal::ModuleInfo> result;
    result.reserve(order_.size());
    for (const auto& id : order_) {
        auto it = handlers_.find(id);
        if (it != handlers_.end())
            result.push_back(it->second->GetModuleInfo());
    }
    return result;
}

// ============================================================
// HalServiceImpl
// ============================================================

HalServiceImpl::HalServiceImpl(std::unique_ptr<ModuleRegistry> registry)
    : registry_(std::move(registry)) {}

grpc::Status HalServiceImpl::Resolve(const std::string& module_id,
                                     IModuleHandler**   out) const {
    *out = registry_->Find(module_id);
    if (!*out)
        return grpc::Status(grpc::StatusCode::NOT_FOUND,
                            "Unknown module_id: \"" + module_id + "\"");
    return grpc::Status::OK;
}

grpc::Status HalServiceImpl::RunSingleShot(grpc::ServerContext*           ctx,
                                            const hal::SingleShotRequest* req,
                                            hal::common::ResultEnvelope*  resp) {
    IModuleHandler* h = nullptr;
    if (auto s = Resolve(req->module_id(), &h); !s.ok()) return s;
    return h->RunSingleShot(ctx, req->module_config(), req->common(), resp);
}

grpc::Status HalServiceImpl::StartLongTerm(
    grpc::ServerContext*                              ctx,
    const hal::LongTermRequest*                       req,
    grpc::ServerWriter<hal::common::ResultEnvelope>*  writer) {
    IModuleHandler* h = nullptr;
    if (auto s = Resolve(req->module_id(), &h); !s.ok()) return s;
    return h->StartLongTerm(ctx, req->module_config(), req->common(), writer);
}

grpc::Status HalServiceImpl::StartMonitoring(
    grpc::ServerContext*                              ctx,
    const hal::MonitoringRequest*                     req,
    grpc::ServerWriter<hal::common::ResultEnvelope>*  writer) {
    IModuleHandler* h = nullptr;
    if (auto s = Resolve(req->module_id(), &h); !s.ok()) return s;
    return h->StartMonitoring(ctx, req->module_config(), req->common(),
                              req->monitoring(), writer);
}

grpc::Status HalServiceImpl::StartEventBased(
    grpc::ServerContext*                              ctx,
    const hal::EventBasedRequest*                     req,
    grpc::ServerWriter<hal::common::ResultEnvelope>*  writer) {
    IModuleHandler* h = nullptr;
    if (auto s = Resolve(req->module_id(), &h); !s.ok()) return s;
    return h->StartEventBased(ctx, req->module_config(), req->common(),
                              req->condition(), writer);
}

grpc::Status HalServiceImpl::Stop(grpc::ServerContext*    /*ctx*/,
                                   const hal::StopRequest* req,
                                   hal::StopResponse*       resp) {
    if (req->module_id().empty()) {
        for (const auto& info : registry_->ListAll()) {
            auto* h = registry_->Find(info.id());
            if (h) h->Stop();
        }
        resp->set_accepted(true);
        resp->set_message("All modules stopped");
        return grpc::Status::OK;
    }
    IModuleHandler* h = nullptr;
    if (auto s = Resolve(req->module_id(), &h); !s.ok()) return s;
    h->Stop();
    resp->set_accepted(true);
    resp->set_message("Stopped " + req->module_id());
    return grpc::Status::OK;
}

grpc::Status HalServiceImpl::Pause(grpc::ServerContext*     /*ctx*/,
                                    const hal::PauseRequest* req,
                                    hal::PauseResponse*      resp) {
    if (req->module_id().empty()) {
        for (const auto& info : registry_->ListAll()) {
            auto* h = registry_->Find(info.id());
            if (h) h->Pause();
        }
        resp->set_accepted(true);
        resp->set_message("All modules paused");
        return grpc::Status::OK;
    }
    IModuleHandler* h = nullptr;
    if (auto s = Resolve(req->module_id(), &h); !s.ok()) return s;
    h->Pause();
    resp->set_accepted(true);
    resp->set_message("Paused " + req->module_id());
    return grpc::Status::OK;
}

grpc::Status HalServiceImpl::Resume(grpc::ServerContext*      /*ctx*/,
                                     const hal::ResumeRequest* req,
                                     hal::ResumeResponse*      resp) {
    if (req->module_id().empty()) {
        for (const auto& info : registry_->ListAll()) {
            auto* h = registry_->Find(info.id());
            if (h) h->Resume();
        }
        resp->set_accepted(true);
        resp->set_message("All modules resumed");
        return grpc::Status::OK;
    }
    IModuleHandler* h = nullptr;
    if (auto s = Resolve(req->module_id(), &h); !s.ok()) return s;
    h->Resume();
    resp->set_accepted(true);
    resp->set_message("Resumed " + req->module_id());
    return grpc::Status::OK;
}

grpc::Status HalServiceImpl::ListModules(grpc::ServerContext*           /*ctx*/,
                                          const hal::ListModulesRequest* /*req*/,
                                          hal::ListModulesResponse*       resp) {
    for (const auto& info : registry_->ListAll())
        *resp->add_modules() = info;
    return grpc::Status::OK;
}

} // namespace server
} // namespace hal
