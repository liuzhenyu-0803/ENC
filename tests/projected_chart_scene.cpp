#include "render/projected_chart_scene.h"

#include <cmath>
#include <iostream>
#include <string_view>

namespace {

bool Expect(bool condition, std::string_view message) {
  if (condition) {
    return true;
  }

  std::cerr << "[navscene-projected-chart-scene] " << message << '\n';
  return false;
}

bool NearlyEqual(float a, float b, float epsilon = 1e-5f) {
  return std::fabs(a - b) <= epsilon;
}

navscene::Viewport BuildViewport() {
  navscene::Viewport viewport;
  viewport.center = {.lat = 15.0, .lon = 15.0};
  viewport.scale_ppm = 1.0;
  viewport.width = 400;
  viewport.height = 300;
  return viewport;
}

navscene::GeoBox BuildCoverage() {
  return navscene::GeoBox{
      .min_lat = 10.0,
      .min_lon = 10.0,
      .max_lat = 20.0,
      .max_lon = 20.0,
  };
}

navscene::render::ChartScene BuildScene() {
  navscene::render::ChartScene scene;

  scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample-a",
      .object_class_code = 42,
      .object_class_acronym = "DEPARE",
      .attributes = {{"DRVAL1", "2.0"}, {"DRVAL2", "6.0"}},
      .outer_ring = {
          {.lat = 12.0, .lon = 12.0},
          {.lat = 12.0, .lon = 18.0},
          {.lat = 18.0, .lon = 18.0},
          {.lat = 18.0, .lon = 12.0},
      },
  });

  scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample-b",
      .object_class_code = 500,
      .object_class_acronym = "M_COVR",
      .outer_ring = {
          {.lat = 11.0, .lon = 11.0},
          {.lat = 11.0, .lon = 19.0},
          {.lat = 19.0, .lon = 19.0},
          {.lat = 19.0, .lon = 11.0},
      },
      .holes = {{
          {.lat = 13.0, .lon = 13.0},
          {.lat = 13.0, .lon = 14.0},
          {.lat = 14.0, .lon = 14.0},
      }},
  });

  scene.polylines.push_back(navscene::render::PolylinePrimitive{
      .dataset_path = "sample-a",
      .object_class_code = 30,
      .object_class_acronym = "COALNE",
      .vertices = {
          {.lat = 12.0, .lon = 10.5},
          {.lat = 13.0, .lon = 12.5},
          {.lat = 14.0, .lon = 14.5},
      },
  });

  scene.points.push_back(navscene::render::PointPrimitive{
      .dataset_path = "sample-a",
      .object_class_code = 129,
      .object_class_acronym = "SOUNDG",
      .position = {.lat = 15.0, .lon = 15.0},
  });

  scene.stats.point_primitive_count = static_cast<uint64_t>(scene.points.size());
  scene.stats.polyline_primitive_count = static_cast<uint64_t>(scene.polylines.size());
  scene.stats.polygon_primitive_count = static_cast<uint64_t>(scene.polygons.size());
  return scene;
}

navscene::render::ChartScene BuildDashedScene() {
  navscene::render::ChartScene scene;
  scene.polylines.push_back(navscene::render::PolylinePrimitive{
      .dataset_path = "sample-c",
      .object_class_code = 901,
      .object_class_acronym = "RECTRC",
      .vertices = {
          {.lat = 12.0, .lon = 12.0},
          {.lat = 18.0, .lon = 18.0},
      },
  });
  scene.stats.polyline_primitive_count = static_cast<uint64_t>(scene.polylines.size());
  return scene;
}

}  // namespace

int main() {
  const auto projected = navscene::render::BuildProjectedChartScene(
      BuildScene(), BuildCoverage(), BuildViewport());

  if (!Expect(projected.has_valid_coverage,
              "Projected scene should report valid coverage for a valid viewport.")) {
    return 1;
  }
  if (!Expect(!projected.requires_complex_polygon_support,
              "Projected scene should triangulate polygons with holes without fallback.")) {
    return 1;
  }
  if (!Expect(!projected.requires_complex_line_support,
              "Projected scene should keep solid-line scenes on the native path.")) {
    return 1;
  }
  if (!Expect(projected.stats.skipped_complex_polygon_count == 0,
              "Projected scene should not skip polygons with holes after triangulation.")) {
    return 1;
  }
  if (!Expect(projected.triangle_vertices.size() == 6 &&
                  projected.triangle_vertices.size() % 3 == 0,
              "Projected scene should emit triangle vertices only for fill-enabled polygons.")) {
    return 1;
  }
  if (!Expect(projected.line_vertices.size() == 26,
              "Scene should include both polygon outlines, the hole outline, and the polyline.")) {
    return 1;
  }
  if (!Expect(projected.point_vertices.size() == 1,
              "Scene should include one projected point vertex.")) {
    return 1;
  }
  if (!Expect(projected.stats.triangle_vertex_count == projected.triangle_vertices.size() &&
                  projected.stats.line_vertex_count == projected.line_vertices.size() &&
                  projected.stats.point_vertex_count == projected.point_vertices.size(),
              "Projected scene stats should match emitted vertex buffers.")) {
    return 1;
  }

  const auto& center_point = projected.point_vertices.front();
  if (!Expect(NearlyEqual(center_point.x, 0.0f) && NearlyEqual(center_point.y, 0.0f),
              "Viewport-centered point should project to clip-space center.")) {
    return 1;
  }
  if (!Expect(center_point.size_px >= 1.0f,
              "Projected point size should stay positive.")) {
    return 1;
  }

  const auto empty = navscene::render::BuildProjectedChartScene(
      BuildScene(), navscene::GeoBox{}, BuildViewport());
  if (!Expect(!empty.has_valid_coverage && empty.triangle_vertices.empty() &&
                  empty.line_vertices.empty() && empty.point_vertices.empty(),
              "Invalid coverage should short-circuit projected scene generation.")) {
    return 1;
  }

  const auto dashed = navscene::render::BuildProjectedChartScene(
      BuildDashedScene(), BuildCoverage(), BuildViewport());
  if (!Expect(dashed.requires_complex_line_support,
              "Dashed portrayal lines should mark the projected scene for raster fallback.")) {
    return 1;
  }

  return 0;
}
