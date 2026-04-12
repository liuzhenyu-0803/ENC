#pragma once

#include "data/s57/model.h"
#include "render/scene.h"

namespace navscene::render {

struct SceneBuildOptions {
  bool show_soundings = true;
  bool show_lights = true;
};

void AppendDatasetToChartScene(const data::s57::DatasetInfo& dataset,
                               const SceneBuildOptions& options,
                               ChartScene* out);

void AppendDatasetToChartScene(const data::s57::DatasetInfo& dataset, ChartScene* out);

}  // namespace navscene::render
