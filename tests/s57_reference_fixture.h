#pragma once

#include "core/default_view.h"

namespace navscene::testsupport {

inline bool IsBundledReferenceDataset(const DatasetDescriptor& descriptor) {
  return descriptor.id == "GB4X0000.000" || descriptor.path.ends_with("GB4X0000.000");
}

inline Viewport MakeBundledReferenceViewport(const DatasetDescriptor& descriptor,
                                             uint32_t width,
                                             uint32_t height,
                                             int padding_pixels = 0) {
  Viewport viewport = core::MakePreferredViewport(descriptor, width, height, padding_pixels);
  if (IsBundledReferenceDataset(descriptor)) {
    viewport.center = GeoPoint{
        .lat = -32.4551,
        .lon = 60.99,
    };
  }
  return viewport;
}

}  // namespace navscene::testsupport
