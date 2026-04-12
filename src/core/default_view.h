#pragma once

#include "navscene/navscene.h"

namespace navscene::core {

Viewport MakePreferredViewport(const DatasetDescriptor& descriptor,
                               uint32_t width,
                               uint32_t height,
                               int padding_pixels = 0);

}  // namespace navscene::core
