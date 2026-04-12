#include "render/polygon_triangulation.h"

#include <mapbox/earcut.hpp>

#include <array>
#include <cmath>
#include <exception>

namespace navscene::render {
namespace {

constexpr double kCoordinateEpsilon = 1e-9;

bool IsSamePoint(const TriangulationPoint2D& a, const TriangulationPoint2D& b) {
  return std::abs(a.x - b.x) <= kCoordinateEpsilon &&
         std::abs(a.y - b.y) <= kCoordinateEpsilon;
}

std::vector<TriangulationPoint2D> NormalizeRing(
    const std::vector<TriangulationPoint2D>& ring) {
  std::vector<TriangulationPoint2D> normalized;
  normalized.reserve(ring.size());

  for (const auto& point : ring) {
    if (!normalized.empty() && IsSamePoint(normalized.back(), point)) {
      continue;
    }
    normalized.push_back(point);
  }

  if (normalized.size() >= 2 && IsSamePoint(normalized.front(), normalized.back())) {
    normalized.pop_back();
  }

  return normalized;
}

}  // namespace

Status TriangulatePolygon2D(const std::vector<TriangulationPoint2D>& outer_ring,
                            const std::vector<std::vector<TriangulationPoint2D>>& holes,
                            TriangulatedPolygon2D* out) {
  if (out == nullptr) {
    return Status{StatusCode::kInvalidArgument,
                  "Triangulated polygon output must not be null."};
  }

  out->vertices.clear();
  out->triangle_indices.clear();

  const auto normalized_outer = NormalizeRing(outer_ring);
  if (normalized_outer.size() < 3) {
    return {};
  }

  using EarcutPoint = std::array<double, 2>;
  std::vector<std::vector<EarcutPoint>> polygon;
  polygon.reserve(1 + holes.size());

  polygon.push_back({});
  polygon.front().reserve(normalized_outer.size());
  for (const auto& point : normalized_outer) {
    polygon.front().push_back({point.x, point.y});
    out->vertices.push_back(point);
  }

  for (const auto& hole : holes) {
    const auto normalized_hole = NormalizeRing(hole);
    if (normalized_hole.size() < 3) {
      continue;
    }

    polygon.push_back({});
    polygon.back().reserve(normalized_hole.size());
    for (const auto& point : normalized_hole) {
      polygon.back().push_back({point.x, point.y});
      out->vertices.push_back(point);
    }
  }

  try {
    out->triangle_indices = mapbox::earcut<uint32_t>(polygon);
  } catch (const std::exception& exception) {
    out->vertices.clear();
    out->triangle_indices.clear();
    return Status{StatusCode::kInternalError, exception.what()};
  } catch (...) {
    out->vertices.clear();
    out->triangle_indices.clear();
    return Status{StatusCode::kInternalError, "Polygon triangulation failed."};
  }

  return {};
}

}  // namespace navscene::render
