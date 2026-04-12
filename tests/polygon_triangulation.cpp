#include "render/polygon_triangulation.h"

#include <iostream>
#include <string_view>
#include <vector>

namespace {

bool Expect(bool condition, std::string_view message) {
  if (condition) {
    return true;
  }

  std::cerr << "[navscene-polygon-triangulation] " << message << '\n';
  return false;
}

using navscene::render::TriangulatedPolygon2D;
using navscene::render::TriangulationPoint2D;

bool ValidateTriangleIndices(const TriangulatedPolygon2D& polygon) {
  if (polygon.triangle_indices.size() % 3 != 0) {
    return false;
  }

  for (const uint32_t index : polygon.triangle_indices) {
    if (index >= polygon.vertices.size()) {
      return false;
    }
  }

  return true;
}

}  // namespace

int main() {
  TriangulatedPolygon2D simple_polygon;
  const auto simple_status = navscene::render::TriangulatePolygon2D(
      {
          {.x = 0.0, .y = 0.0},
          {.x = 10.0, .y = 0.0},
          {.x = 10.0, .y = 10.0},
          {.x = 0.0, .y = 10.0},
      },
      {},
      &simple_polygon);
  if (!Expect(simple_status.ok(), "Simple polygon triangulation should succeed.")) {
    return 1;
  }
  if (!Expect(simple_polygon.vertices.size() == 4,
              "Simple polygon should retain four unique vertices.")) {
    return 1;
  }
  if (!Expect(simple_polygon.triangle_indices.size() == 6,
              "Simple quad should triangulate into two triangles.")) {
    return 1;
  }
  if (!Expect(ValidateTriangleIndices(simple_polygon),
              "Simple polygon triangle indices should be valid.")) {
    return 1;
  }

  TriangulatedPolygon2D holed_polygon;
  const auto holed_status = navscene::render::TriangulatePolygon2D(
      {
          {.x = 0.0, .y = 0.0},
          {.x = 12.0, .y = 0.0},
          {.x = 12.0, .y = 12.0},
          {.x = 0.0, .y = 12.0},
      },
      {{
          {.x = 4.0, .y = 4.0},
          {.x = 8.0, .y = 4.0},
          {.x = 6.0, .y = 8.0},
      }},
      &holed_polygon);
  if (!Expect(holed_status.ok(), "Polygon with a hole should triangulate successfully.")) {
    return 1;
  }
  if (!Expect(!holed_polygon.triangle_indices.empty(),
              "Polygon with a hole should emit triangles.")) {
    return 1;
  }
  if (!Expect(ValidateTriangleIndices(holed_polygon),
              "Polygon-with-hole triangle indices should be valid.")) {
    return 1;
  }

  const auto null_status = navscene::render::TriangulatePolygon2D({}, {}, nullptr);
  if (!Expect(!null_status.ok(),
              "Triangulation should reject a null output pointer.")) {
    return 1;
  }

  return 0;
}
