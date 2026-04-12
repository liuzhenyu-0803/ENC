#pragma once

#include "navscene/navscene.h"

#include <cstdint>
#include <vector>

namespace navscene::render {

struct TriangulationPoint2D {
  double x = 0.0;
  double y = 0.0;
};

struct TriangulatedPolygon2D {
  std::vector<TriangulationPoint2D> vertices;
  std::vector<uint32_t> triangle_indices;
};

Status TriangulatePolygon2D(const std::vector<TriangulationPoint2D>& outer_ring,
                            const std::vector<std::vector<TriangulationPoint2D>>& holes,
                            TriangulatedPolygon2D* out);

}  // namespace navscene::render
