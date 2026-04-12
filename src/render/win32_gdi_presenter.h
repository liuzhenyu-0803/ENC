#pragma once

#include "navscene/navscene.h"
#include "portrayal/scene.h"
#include "render/scene.h"

namespace navscene::render {

Status PresentChartSceneWin32Gdi(const ChartScene& scene,
                                 const GeoBox& reference_coverage,
                                 const Viewport& viewport,
                                 const NativeSurfaceDesc& surface);

Status PresentChartSceneWin32Gdi(const portrayal::PortrayalScene& scene,
                                 const GeoBox& reference_coverage,
                                 const Viewport& viewport,
                                 const NativeSurfaceDesc& surface);

}  // namespace navscene::render
