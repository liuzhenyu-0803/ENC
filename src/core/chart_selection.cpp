#include "core/chart_selection.h"
#include "geo/mercator_projection.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <unordered_set>

namespace navscene::core {
namespace {

constexpr int kViewportPaddingPixels = 48;
constexpr double kCoordinateEpsilon = 1e-9;

double Span(double min_value, double max_value) {
  return std::max(max_value - min_value, 0.0);
}

double CoverageArea(const GeoBox& coverage) {
  return Span(coverage.min_lon, coverage.max_lon) *
         Span(coverage.min_lat, coverage.max_lat);
}

bool Intersects(const GeoBox& lhs, const GeoBox& rhs) {
  if (!HasValidCoverage(lhs) || !HasValidCoverage(rhs)) {
    return false;
  }

  return lhs.min_lon < rhs.max_lon && lhs.max_lon > rhs.min_lon &&
         lhs.min_lat < rhs.max_lat && lhs.max_lat > rhs.min_lat;
}

bool ContainsPoint(const GeoBox& coverage, const GeoPoint& point) {
  if (!HasValidCoverage(coverage)) {
    return false;
  }

  return point.lon >= coverage.min_lon - kCoordinateEpsilon &&
         point.lon <= coverage.max_lon + kCoordinateEpsilon &&
         point.lat >= coverage.min_lat - kCoordinateEpsilon &&
         point.lat <= coverage.max_lat + kCoordinateEpsilon;
}

int EffectiveCompilationScale(const data::s57::DatasetInfo& dataset,
                              double fallback_scale) {
  if (dataset.descriptor.compilation_scale > 0) {
    return dataset.descriptor.compilation_scale;
  }

  return static_cast<int>(std::max(std::round(fallback_scale), 1.0));
}

double SuitabilityPenalty(const data::s57::DatasetInfo& dataset, double target_scale) {
  if (target_scale <= 0.0) {
    return 0.0;
  }

  if (dataset.descriptor.compilation_scale <= 0) {
    return 8.0;
  }

  const double ratio =
      static_cast<double>(dataset.descriptor.compilation_scale) / target_scale;
  double penalty = std::abs(std::log(std::max(ratio, 1e-6)));
  if (ratio > 1.0) {
    // Favor more detailed charts once the viewport asks for a tighter scale.
    penalty += std::min(std::log(ratio), 1.5);
  }
  return penalty;
}

bool CompareRenderOrder(const data::s57::DatasetInfo* lhs,
                        const data::s57::DatasetInfo* rhs,
                        double fallback_scale) {
  const int lhs_scale = EffectiveCompilationScale(*lhs, fallback_scale);
  const int rhs_scale = EffectiveCompilationScale(*rhs, fallback_scale);
  if (lhs_scale != rhs_scale) {
    return lhs_scale > rhs_scale;
  }

  const double lhs_area = CoverageArea(lhs->descriptor.coverage);
  const double rhs_area = CoverageArea(rhs->descriptor.coverage);
  if (std::abs(lhs_area - rhs_area) > kCoordinateEpsilon) {
    return lhs_area > rhs_area;
  }

  return lhs->descriptor.path < rhs->descriptor.path;
}

bool BetterSelectionCandidate(const data::s57::DatasetInfo* lhs,
                              const data::s57::DatasetInfo* rhs,
                              double target_scale) {
  const double lhs_penalty = SuitabilityPenalty(*lhs, target_scale);
  const double rhs_penalty = SuitabilityPenalty(*rhs, target_scale);
  if (std::abs(lhs_penalty - rhs_penalty) > kCoordinateEpsilon) {
    return lhs_penalty < rhs_penalty;
  }

  const int lhs_scale = EffectiveCompilationScale(*lhs, target_scale);
  const int rhs_scale = EffectiveCompilationScale(*rhs, target_scale);
  if (lhs_scale != rhs_scale) {
    return lhs_scale < rhs_scale;
  }

  const double lhs_area = CoverageArea(lhs->descriptor.coverage);
  const double rhs_area = CoverageArea(rhs->descriptor.coverage);
  if (std::abs(lhs_area - rhs_area) > kCoordinateEpsilon) {
    return lhs_area < rhs_area;
  }

  return lhs->descriptor.path < rhs->descriptor.path;
}

std::vector<const data::s57::DatasetInfo*> SortForRender(
    std::vector<const data::s57::DatasetInfo*> datasets,
    double fallback_scale) {
  std::sort(datasets.begin(),
            datasets.end(),
            [&](const data::s57::DatasetInfo* lhs,
                const data::s57::DatasetInfo* rhs) {
              return CompareRenderOrder(lhs, rhs, fallback_scale);
            });
  return datasets;
}

}  // namespace

bool HasValidCoverage(const GeoBox& coverage) {
  return geo::HasValidCoverage(coverage);
}

GeoBox ComputeViewportCoverage(const GeoBox& reference_coverage,
                               const Viewport& viewport) {
  return geo::ComputeViewportCoverage(reference_coverage, viewport, kViewportPaddingPixels);
}

double EstimateViewportDisplayScale(const GeoBox& viewport_coverage,
                                    const Viewport& viewport) {
  return geo::EstimateViewportDisplayScale(
      viewport_coverage, viewport, kViewportPaddingPixels);
}

ChartSelectionResult SelectChartsForViewport(
    const std::vector<const data::s57::DatasetInfo*>& candidates,
    const GeoBox& reference_coverage,
    const Viewport& viewport) {
  ChartSelectionResult result;

  std::vector<const data::s57::DatasetInfo*> eligible;
  eligible.reserve(candidates.size());
  for (const auto* dataset : candidates) {
    if (dataset == nullptr || !HasValidCoverage(dataset->descriptor.coverage)) {
      continue;
    }
    eligible.push_back(dataset);
  }

  if (eligible.empty()) {
    return result;
  }

  result.viewport_coverage = ComputeViewportCoverage(reference_coverage, viewport);
  result.has_viewport_coverage = HasValidCoverage(result.viewport_coverage);
  result.estimated_display_scale =
      EstimateViewportDisplayScale(result.viewport_coverage, viewport);

  if (!result.has_viewport_coverage || result.estimated_display_scale <= 0.0) {
    result.selected_datasets =
        SortForRender(std::move(eligible), std::max(result.estimated_display_scale, 1.0));
    return result;
  }

  std::vector<const data::s57::DatasetInfo*> intersecting;
  intersecting.reserve(eligible.size());
  for (const auto* dataset : eligible) {
    if (Intersects(dataset->descriptor.coverage, result.viewport_coverage)) {
      intersecting.push_back(dataset);
    }
  }

  if (intersecting.empty()) {
    result.selected_datasets =
        SortForRender(std::move(eligible), result.estimated_display_scale);
    return result;
  }

  const int columns =
      std::clamp(static_cast<int>(viewport.width / 96), 4, 16);
  const int rows =
      std::clamp(static_cast<int>(viewport.height / 96), 4, 12);
  std::unordered_set<std::string> selected_paths;
  std::vector<const data::s57::DatasetInfo*> selected;
  selected.reserve(intersecting.size());

  const double lon_span =
      result.viewport_coverage.max_lon - result.viewport_coverage.min_lon;
  const double lat_span =
      result.viewport_coverage.max_lat - result.viewport_coverage.min_lat;

  for (int row = 0; row < rows; ++row) {
    for (int column = 0; column < columns; ++column) {
      const GeoPoint sample{
          .lat = result.viewport_coverage.min_lat +
                 (static_cast<double>(row) + 0.5) / static_cast<double>(rows) * lat_span,
          .lon = result.viewport_coverage.min_lon +
                 (static_cast<double>(column) + 0.5) /
                     static_cast<double>(columns) * lon_span,
      };

      const data::s57::DatasetInfo* winner = nullptr;
      for (const auto* dataset : intersecting) {
        if (!ContainsPoint(dataset->descriptor.coverage, sample)) {
          continue;
        }

        if (winner == nullptr ||
            BetterSelectionCandidate(dataset, winner, result.estimated_display_scale)) {
          winner = dataset;
        }
      }

      if (winner != nullptr &&
          selected_paths.insert(winner->descriptor.path).second) {
        selected.push_back(winner);
      }
    }
  }

  if (selected.empty()) {
    const data::s57::DatasetInfo* best = nullptr;
    for (const auto* dataset : intersecting) {
      if (best == nullptr ||
          BetterSelectionCandidate(dataset, best, result.estimated_display_scale)) {
        best = dataset;
      }
    }
    if (best != nullptr) {
      selected.push_back(best);
    }
  }

  result.selected_datasets =
      SortForRender(std::move(selected), result.estimated_display_scale);
  return result;
}

}  // namespace navscene::core
