#include "core/chart_selection.h"
#include "render/chart_scene_builder.h"
#include "render/scene.h"

#include <iostream>
#include <string_view>
#include <vector>

namespace {

bool Expect(bool condition, std::string_view message) {
  if (condition) {
    return true;
  }

  std::cerr << "[navscene-chart-selection] " << message << '\n';
  return false;
}

navscene::data::s57::DatasetInfo MakeDataset(std::string_view path,
                                             int compilation_scale,
                                             navscene::GeoBox coverage) {
  navscene::data::s57::DatasetInfo dataset;
  dataset.descriptor.path = std::string(path);
  dataset.descriptor.compilation_scale = compilation_scale;
  dataset.descriptor.coverage = coverage;
  dataset.geometry_loaded = true;

  navscene::data::s57::AreaFeatureGeometry area;
  area.object_class_acronym = "DEPARE";
  area.polygons.push_back(navscene::data::s57::PolygonGeometry{
      .outer_ring =
          {
              {.lat = coverage.min_lat, .lon = coverage.min_lon},
              {.lat = coverage.min_lat, .lon = coverage.max_lon},
              {.lat = coverage.max_lat, .lon = coverage.max_lon},
              {.lat = coverage.max_lat, .lon = coverage.min_lon},
          },
  });
  dataset.geometry.area_features.push_back(std::move(area));
  dataset.geometry.summary.area_feature_count = 1;
  dataset.geometry.summary.area_polygon_count = 1;
  return dataset;
}

std::vector<const navscene::data::s57::DatasetInfo*> MakePointers(
    const std::vector<navscene::data::s57::DatasetInfo>& datasets) {
  std::vector<const navscene::data::s57::DatasetInfo*> pointers;
  pointers.reserve(datasets.size());
  for (const auto& dataset : datasets) {
    pointers.push_back(&dataset);
  }
  return pointers;
}

bool ExpectSelectedPaths(
    const navscene::core::ChartSelectionResult& result,
    const std::vector<std::string_view>& expected_paths,
    std::string_view label) {
  if (!Expect(result.selected_datasets.size() == expected_paths.size(),
              std::string(label) + " selected dataset count mismatch.")) {
    std::cerr << "  expected:";
    for (const auto& path : expected_paths) {
      std::cerr << ' ' << path;
    }
    std::cerr << "\n  actual:";
    for (const auto* dataset : result.selected_datasets) {
      std::cerr << ' ' << (dataset == nullptr ? "(null)" : dataset->descriptor.path);
    }
    std::cerr << '\n';
    return false;
  }

  for (size_t index = 0; index < expected_paths.size(); ++index) {
    if (!Expect(result.selected_datasets[index] != nullptr &&
                    result.selected_datasets[index]->descriptor.path == expected_paths[index],
                std::string(label) + " selected dataset order mismatch.")) {
      std::cerr << "  expected:";
      for (const auto& path : expected_paths) {
        std::cerr << ' ' << path;
      }
      std::cerr << "\n  actual:";
      for (const auto* dataset : result.selected_datasets) {
        std::cerr << ' ' << (dataset == nullptr ? "(null)" : dataset->descriptor.path);
      }
      std::cerr << '\n';
      return false;
    }
  }

  return true;
}

}  // namespace

int main() {
  const navscene::GeoBox reference_coverage{
      .min_lat = 0.0,
      .min_lon = 0.0,
      .max_lat = 10.0,
      .max_lon = 10.0,
  };
  const navscene::Viewport viewport_base{
      .center = {.lat = 5.0, .lon = 5.0},
      .scale_ppm = 1.0,
      .rotation_rad = 0.0,
      .width = 1024,
      .height = 768,
  };

  const std::vector<navscene::data::s57::DatasetInfo> datasets = {
      MakeDataset("A_coarse.000", 3000000, reference_coverage),
      MakeDataset("B_left_fine.000",
                  300000,
                  navscene::GeoBox{
                      .min_lat = 0.0,
                      .min_lon = 0.0,
                      .max_lat = 10.0,
                      .max_lon = 5.0,
                  }),
      MakeDataset("C_right_fine.000",
                  300000,
                  navscene::GeoBox{
                      .min_lat = 0.0,
                      .min_lon = 5.0,
                      .max_lat = 10.0,
                      .max_lon = 10.0,
                  }),
  };
  const auto candidates = MakePointers(datasets);

  const auto full_view = navscene::core::SelectChartsForViewport(
      candidates, reference_coverage, viewport_base);
  if (!Expect(full_view.has_viewport_coverage,
              "Selection should compute viewport coverage for a valid viewport.")) {
    return 1;
  }
  if (!Expect(full_view.estimated_display_scale > 1000000.0,
              "Selection should estimate a plausible display scale.")) {
    return 1;
  }
  if (!ExpectSelectedPaths(full_view, {"A_coarse.000"}, "zoomed-out coarse preference")) {
    return 1;
  }

  navscene::Viewport left_zoom = viewport_base;
  left_zoom.center.lon = 2.5;
  left_zoom.scale_ppm = 6.0;
  const auto left_view = navscene::core::SelectChartsForViewport(
      candidates, reference_coverage, left_zoom);
  if (!ExpectSelectedPaths(left_view, {"B_left_fine.000"}, "zoomed-in fine preference")) {
    return 1;
  }

  const std::vector<navscene::data::s57::DatasetInfo> gap_fill_datasets = {
      MakeDataset("A_coarse.000", 3000000, reference_coverage),
      MakeDataset("B_left_fine.000",
                  300000,
                  navscene::GeoBox{
                      .min_lat = 0.0,
                      .min_lon = 0.0,
                      .max_lat = 10.0,
                      .max_lon = 5.0,
                  }),
  };
  const auto gap_fill_candidates = MakePointers(gap_fill_datasets);
  navscene::Viewport boundary_zoom = viewport_base;
  boundary_zoom.center.lon = 4.75;
  boundary_zoom.scale_ppm = 6.0;
  const auto boundary_view = navscene::core::SelectChartsForViewport(
      gap_fill_candidates, reference_coverage, boundary_zoom);
  if (!ExpectSelectedPaths(boundary_view,
                           {"A_coarse.000", "B_left_fine.000"},
                           "boundary quilt fill")) {
    return 1;
  }

  const std::vector<navscene::data::s57::DatasetInfo> tie_datasets = {
      MakeDataset("AA_first.000", 800000, reference_coverage),
      MakeDataset("ZZ_second.000", 800000, reference_coverage),
  };
  const auto tie_candidates = MakePointers(tie_datasets);
  const auto tie_view = navscene::core::SelectChartsForViewport(
      tie_candidates, reference_coverage, viewport_base);
  if (!ExpectSelectedPaths(tie_view, {"AA_first.000"}, "deterministic tie break")) {
    return 1;
  }

  navscene::render::ChartScene scene;
  for (const auto* dataset : boundary_view.selected_datasets) {
    navscene::render::AppendDatasetToChartScene(*dataset, &scene);
  }
  if (!Expect(scene.polygons.size() == 2,
              "Boundary quilt scene should contain both coarse and fine polygons.")) {
    return 1;
  }
  if (!Expect(scene.polygons[0].dataset_path == "A_coarse.000" &&
                  scene.polygons[1].dataset_path == "B_left_fine.000",
              "Scene composition should keep coarse-to-fine render order.")) {
    return 1;
  }

  return 0;
}
