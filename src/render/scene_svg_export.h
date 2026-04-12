#pragma once

#include "portrayal/scene.h"
#include "render/scene.h"

#include <cstdint>
#include <string>

namespace navscene::render {

struct SvgExportOptions {
  uint32_t width = 1024;
  uint32_t height = 768;
  uint32_t padding = 24;
};

std::string ExportChartSceneToSvg(const ChartScene& scene,
                                  const SvgExportOptions& options = {});

std::string ExportChartSceneToSvg(const portrayal::PortrayalScene& scene,
                                  const SvgExportOptions& options = {});

}  // namespace navscene::render
