#pragma once

#include "data/s57/model.h"
#include "navscene/navscene.h"

#include <vector>

namespace navscene::core {

struct ChartSelectionResult {
  GeoBox viewport_coverage{};
  bool has_viewport_coverage = false;
  double estimated_display_scale = 0.0;
  std::vector<const data::s57::DatasetInfo*> selected_datasets;
};

bool HasValidCoverage(const GeoBox& coverage);

GeoBox ComputeViewportCoverage(const GeoBox& reference_coverage,
                               const Viewport& viewport);

double EstimateViewportDisplayScale(const GeoBox& viewport_coverage,
                                    const Viewport& viewport);

ChartSelectionResult SelectChartsForViewport(
    const std::vector<const data::s57::DatasetInfo*>& candidates,
    const GeoBox& reference_coverage,
    const Viewport& viewport);

}  // namespace navscene::core
