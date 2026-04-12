#pragma once

#include "navscene/navscene.h"
#include "portrayal/scene.h"
#include "render/scene.h"

#include <cstdint>
#include <vector>

namespace navscene::render {

struct SoftwareRasterImage {
  uint32_t width = 0;
  uint32_t height = 0;
  std::vector<uint8_t> bgra_pixels;
};

Status RasterizeChartSceneWin32(const ChartScene& scene,
                                const GeoBox& reference_coverage,
                                const Viewport& viewport,
                                uint32_t width,
                                uint32_t height,
                                SoftwareRasterImage* out);

Status RasterizeChartSceneWin32(const portrayal::PortrayalScene& scene,
                                const GeoBox& reference_coverage,
                                const Viewport& viewport,
                                uint32_t width,
                                uint32_t height,
                                SoftwareRasterImage* out);

Status PresentSoftwareRasterWin32(const SoftwareRasterImage& image,
                                  const NativeSurfaceDesc& surface);

}  // namespace navscene::render
