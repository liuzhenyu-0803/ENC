#include "render/scene_signature.h"

#include <algorithm>
#include <iostream>
#include <string_view>

namespace {

bool Expect(bool condition, std::string_view message) {
  if (condition) {
    return true;
  }

  std::cerr << "[navscene-scene-signature] " << message << '\n';
  return false;
}

navscene::render::ChartScene BuildReferenceScene() {
  navscene::render::ChartScene scene;

  scene.points.push_back(navscene::render::PointPrimitive{
      .dataset_path = "sample-a",
      .object_class_code = 129,
      .object_class_acronym = "SOUNDG",
      .position = navscene::GeoPoint{.lat = 20.0, .lon = 120.0},
  });
  scene.points.push_back(navscene::render::PointPrimitive{
      .dataset_path = "sample-a",
      .object_class_code = 72,
      .object_class_acronym = "LNDELV",
      .position = navscene::GeoPoint{.lat = 21.0, .lon = 121.0},
  });

  scene.polylines.push_back(navscene::render::PolylinePrimitive{
      .dataset_path = "sample-a",
      .object_class_code = 30,
      .object_class_acronym = "COALNE",
      .vertices = {
          {.lat = 20.0, .lon = 120.0},
          {.lat = 20.5, .lon = 120.5},
          {.lat = 21.0, .lon = 121.0},
      },
  });

  scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample-a",
      .object_class_code = 42,
      .object_class_acronym = "DEPARE",
      .outer_ring = {
          {.lat = 19.0, .lon = 119.0},
          {.lat = 19.0, .lon = 122.0},
          {.lat = 22.0, .lon = 122.0},
          {.lat = 22.0, .lon = 119.0},
      },
      .holes = {{
          {.lat = 20.0, .lon = 120.0},
          {.lat = 20.0, .lon = 121.0},
          {.lat = 21.0, .lon = 121.0},
      }},
  });

  scene.stats.point_primitive_count = static_cast<uint64_t>(scene.points.size());
  scene.stats.polyline_primitive_count = static_cast<uint64_t>(scene.polylines.size());
  scene.stats.polygon_primitive_count = static_cast<uint64_t>(scene.polygons.size());
  return scene;
}

}  // namespace

int main() {
  auto reference = BuildReferenceScene();
  auto reordered = BuildReferenceScene();

  std::reverse(reordered.points.begin(), reordered.points.end());
  std::reverse(reordered.polylines.begin(), reordered.polylines.end());
  std::reverse(reordered.polygons.begin(), reordered.polygons.end());

  const auto reference_signature = navscene::render::BuildChartSceneSignature(reference);
  const auto reordered_signature = navscene::render::BuildChartSceneSignature(reordered);

  if (!Expect(reference_signature.fingerprint64 == reordered_signature.fingerprint64,
              "Scene fingerprint should be stable across primitive ordering.")) {
    return 1;
  }
  if (!Expect(reference_signature.class_buckets.size() == 4,
              "Scene signature should aggregate the expected class buckets.")) {
    return 1;
  }
  if (!Expect(reference_signature.total_vertex_count == 12,
              "Scene signature should include point, polyline and polygon vertices.")) {
    return 1;
  }
  if (!Expect(reference_signature.has_coverage,
              "Scene signature should compute coverage when geometry is present.")) {
    return 1;
  }
  if (!Expect(reference_signature.coverage.min_lat == 19.0 &&
                  reference_signature.coverage.max_lat == 22.0 &&
                  reference_signature.coverage.min_lon == 119.0 &&
                  reference_signature.coverage.max_lon == 122.0,
              "Scene signature coverage should match the merged scene extent.")) {
    return 1;
  }

  return 0;
}
