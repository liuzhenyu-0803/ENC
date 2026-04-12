#pragma once

#include "navscene/navscene.h"
#include "portrayal/scene.h"
#include "render/scene.h"

#include <cstdint>
#include <vector>

namespace navscene::render {

struct ProjectedColorVertex {
  float x = 0.0f;
  float y = 0.0f;
  float r = 1.0f;
  float g = 1.0f;
  float b = 1.0f;
  float a = 1.0f;
};

struct ProjectedPointVertex {
  float x = 0.0f;
  float y = 0.0f;
  float size_px = 1.0f;
  float r = 1.0f;
  float g = 1.0f;
  float b = 1.0f;
  float a = 1.0f;
};

struct ProjectedChartSceneStats {
  uint64_t triangle_vertex_count = 0;
  uint64_t line_vertex_count = 0;
  uint64_t point_vertex_count = 0;
  uint64_t skipped_complex_polygon_count = 0;
};

struct ProjectedChartScene {
  std::vector<ProjectedColorVertex> triangle_vertices;
  std::vector<ProjectedColorVertex> line_vertices;
  std::vector<ProjectedPointVertex> point_vertices;
  ProjectedChartSceneStats stats;
  bool has_valid_coverage = false;
  bool requires_complex_polygon_support = false;
  bool requires_complex_line_support = false;
};

ProjectedChartScene BuildProjectedChartScene(const ChartScene& scene,
                                             const GeoBox& reference_coverage,
                                             const Viewport& viewport);

ProjectedChartScene BuildProjectedChartScene(const portrayal::PortrayalScene& scene,
                                             const GeoBox& reference_coverage,
                                             const Viewport& viewport);

}  // namespace navscene::render
