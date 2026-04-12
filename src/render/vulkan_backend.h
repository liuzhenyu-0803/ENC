#pragma once

#include "render/backend.h"

#include <memory>

namespace navscene::render {

std::unique_ptr<IRendererBackend> CreateVulkanRendererBackend();

}  // namespace navscene::render
