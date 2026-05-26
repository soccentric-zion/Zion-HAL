#pragma once

// module_registry_all.h — factory that builds a ModuleRegistry with all 35 HAL modules.

#include "server/hal_service_impl.h"
#include <memory>

namespace hal {
namespace server {

// Create and return a ModuleRegistry pre-populated with all 35 HAL modules.
// To add a new module: add its TypedModuleHandler registration in
// module_registry_all.cpp — no other server-side file needs to change.
std::unique_ptr<ModuleRegistry> CreateDefaultRegistry();

} // namespace server
} // namespace hal
