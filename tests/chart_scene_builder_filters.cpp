#include "data/s57/geometry.h"
#include "data/s57/model.h"
#include "render/chart_scene_builder.h"

#include <iostream>
#include <string_view>

namespace {

bool Expect(bool condition, std::string_view message) {
  if (condition) {
    return true;
  }

  std::cerr << "[navscene-chart-scene-builder-filters] " << message << '\n';
  return false;
}

navscene::data::s57::DatasetInfo BuildDataset() {
  navscene::data::s57::DatasetInfo dataset;
  dataset.descriptor.path = "sample.000";

  navscene::data::s57::AreaFeatureGeometry depare;
  depare.object_class_acronym = "DEPARE";
  depare.polygons.push_back(navscene::data::s57::PolygonGeometry{
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
          {.lat = 1.0, .lon = 0.0},
      },
  });
  dataset.geometry.area_features.push_back(std::move(depare));

  navscene::data::s57::AreaFeatureGeometry coverage;
  coverage.object_class_acronym = "M_COVR";
  coverage.polygons.push_back(navscene::data::s57::PolygonGeometry{
      .outer_ring = {
          {.lat = -1.0, .lon = -1.0},
          {.lat = -1.0, .lon = 2.0},
          {.lat = 2.0, .lon = 2.0},
          {.lat = 2.0, .lon = -1.0},
      },
  });
  dataset.geometry.area_features.push_back(std::move(coverage));

  navscene::data::s57::PointFeatureGeometry sounding;
  sounding.object_class_acronym = "SOUNDG";
  sounding.points.push_back(navscene::data::s57::PointGeometry{
      .position = {.lat = 0.5, .lon = 0.5},
  });
  dataset.geometry.point_features.push_back(std::move(sounding));

  navscene::data::s57::PointFeatureGeometry buoy;
  buoy.object_class_acronym = "BOYLAT";
  buoy.points.push_back(navscene::data::s57::PointGeometry{
      .position = {.lat = 0.75, .lon = 0.75},
  });
  dataset.geometry.point_features.push_back(std::move(buoy));

  return dataset;
}

}  // namespace

int main() {
  const auto dataset = BuildDataset();

  navscene::render::ChartScene scene_default;
  navscene::render::AppendDatasetToChartScene(dataset, &scene_default);
  if (!Expect(scene_default.polygons.size() == 2,
              "Raw chart scene build should preserve both regular and metadata polygons.")) {
    return 1;
  }
  if (!Expect(scene_default.points.size() == 2,
              "Raw chart scene build should preserve both soundings and navigation points.")) {
    return 1;
  }

  navscene::render::ChartScene scene_no_soundings;
  navscene::render::AppendDatasetToChartScene(
      dataset,
      navscene::render::SceneBuildOptions{
          .show_soundings = false,
          .show_lights = true,
      },
      &scene_no_soundings);
  if (!Expect(scene_no_soundings.polygons.size() == 2,
              "Scene build options should not remove metadata areas from the raw scene.")) {
    return 1;
  }
  if (!Expect(scene_no_soundings.points.size() == 2,
              "Scene build options should not remove point features from the raw scene.")) {
    return 1;
  }

  navscene::render::ChartScene scene_no_lights;
  navscene::render::AppendDatasetToChartScene(
      dataset,
      navscene::render::SceneBuildOptions{
          .show_soundings = false,
          .show_lights = false,
      },
      &scene_no_lights);
  if (!Expect(scene_no_lights.points.size() == 2,
              "Portrayal, not raw scene building, should own light and sounding filtering.")) {
    return 1;
  }

  return 0;
}
