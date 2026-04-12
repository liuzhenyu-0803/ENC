#pragma once

#include "navscene/navscene.h"
#include "render/scene.h"

#include <cstdint>
#include <string>
#include <vector>

namespace navscene::render {

struct ChartSceneClassBucket {
  char primitive_kind = '?';
  std::string object_class_acronym;
  uint64_t primitive_count = 0;
  uint64_t vertex_count = 0;
};

struct ChartSceneSignature {
  uint64_t point_primitive_count = 0;
  uint64_t polyline_primitive_count = 0;
  uint64_t polygon_primitive_count = 0;
  uint64_t total_vertex_count = 0;
  GeoBox coverage{};
  bool has_coverage = false;
  std::vector<ChartSceneClassBucket> class_buckets;
  uint64_t fingerprint64 = 0;
};

ChartSceneSignature BuildChartSceneSignature(const ChartScene& scene);
std::string FormatChartSceneSignature(const ChartSceneSignature& signature);

}  // namespace navscene::render
