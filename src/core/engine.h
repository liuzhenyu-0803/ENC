#pragma once

#include "navscene/navscene.h"

#include <memory>

namespace navscene::internal {

std::unique_ptr<IEngine> CreateEngineImpl(const EngineConfig& config);

}  // namespace navscene::internal
