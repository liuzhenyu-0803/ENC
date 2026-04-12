#include "geo/mercator_projection.h"
#include "render/projected_chart_scene.h"

#include "portrayal/engine.h"
#include "render/polygon_triangulation.h"

#include <algorithm>
#include <cmath>

namespace navscene::render {
namespace {

constexpr int kPaddingPixels = 0;
constexpr double kCoordinateEpsilon = 1e-9;

portrayal::DisplaySettings BuildCompatibilitySettings() {
  auto settings = portrayal::MakeDisplaySettings(DisplayOptions{});
  settings.display_category = DisplayCategory::kAll;
  settings.show_meta = true;
  settings.show_quality_of_data = true;
  return settings;
}

struct PixelPoint {
  float x = 0.0f;
  float y = 0.0f;
};

bool HasValidCoverage(const GeoBox& coverage) {
  return geo::HasValidCoverage(coverage);
}

bool IsSamePoint(const GeoPoint& a, const GeoPoint& b) {
  return std::abs(a.lat - b.lat) <= kCoordinateEpsilon &&
         std::abs(a.lon - b.lon) <= kCoordinateEpsilon;
}

std::vector<GeoPoint> NormalizeRing(const std::vector<GeoPoint>& ring) {
  std::vector<GeoPoint> normalized = ring;
  if (normalized.size() >= 2 && IsSamePoint(normalized.front(), normalized.back())) {
    normalized.pop_back();
  }
  return normalized;
}

PixelPoint ProjectToPixels(const GeoPoint& point,
                           const GeoBox& reference_coverage,
                           const Viewport& viewport) {
  const auto pixel = geo::GeoToPixel(reference_coverage, viewport, point, kPaddingPixels);
  return PixelPoint{
      .x = static_cast<float>(pixel.first),
      .y = static_cast<float>(pixel.second),
  };
}

std::vector<TriangulationPoint2D> ProjectRingToPixels(
    const std::vector<GeoPoint>& ring,
    const GeoBox& reference_coverage,
    const Viewport& viewport) {
  std::vector<TriangulationPoint2D> projected_ring;
  projected_ring.reserve(ring.size());
  for (const auto& point : ring) {
    const PixelPoint pixel = ProjectToPixels(point, reference_coverage, viewport);
    projected_ring.push_back(TriangulationPoint2D{
        .x = static_cast<double>(pixel.x),
        .y = static_cast<double>(pixel.y),
    });
  }
  return projected_ring;
}

ProjectedColorVertex MakeColorVertex(const PixelPoint& pixel,
                                     const portrayal::Rgb8& color,
                                     const Viewport& viewport) {
  const float width = static_cast<float>(viewport.width);
  const float height = static_cast<float>(viewport.height);
  return ProjectedColorVertex{
      .x = width > 0.0f ? (pixel.x / width) * 2.0f - 1.0f : 0.0f,
      .y = height > 0.0f ? 1.0f - (pixel.y / height) * 2.0f : 0.0f,
      .r = static_cast<float>(color.r) / 255.0f,
      .g = static_cast<float>(color.g) / 255.0f,
      .b = static_cast<float>(color.b) / 255.0f,
      .a = 1.0f,
  };
}

ProjectedPointVertex MakePointVertex(const PixelPoint& pixel,
                                     const portrayal::PointSymbolStyle& style,
                                     const Viewport& viewport) {
  const float width = static_cast<float>(viewport.width);
  const float height = static_cast<float>(viewport.height);
  return ProjectedPointVertex{
      .x = width > 0.0f ? (pixel.x / width) * 2.0f - 1.0f : 0.0f,
      .y = height > 0.0f ? 1.0f - (pixel.y / height) * 2.0f : 0.0f,
      .size_px = static_cast<float>(std::max(style.size_px, 1)),
      .r = static_cast<float>(style.fill.r) / 255.0f,
      .g = static_cast<float>(style.fill.g) / 255.0f,
      .b = static_cast<float>(style.fill.b) / 255.0f,
      .a = 1.0f,
  };
}

void AppendPolylineVertices(const std::vector<GeoPoint>& input_vertices,
                            const portrayal::Rgb8& color,
                            const GeoBox& reference_coverage,
                            const Viewport& viewport,
                            std::vector<ProjectedColorVertex>* out) {
  if (out == nullptr || input_vertices.size() < 2) {
    return;
  }

  for (size_t index = 1; index < input_vertices.size(); ++index) {
    out->push_back(MakeColorVertex(
        ProjectToPixels(input_vertices[index - 1], reference_coverage, viewport),
        color,
        viewport));
    out->push_back(MakeColorVertex(
        ProjectToPixels(input_vertices[index], reference_coverage, viewport),
        color,
        viewport));
  }
}

}  // namespace

ProjectedChartScene BuildProjectedChartScene(const ChartScene& scene,
                                             const GeoBox& reference_coverage,
                                             const Viewport& viewport) {
  return BuildProjectedChartScene(
      portrayal::BuildPortrayalScene(scene, BuildCompatibilitySettings()),
      reference_coverage,
      viewport);
}

ProjectedChartScene BuildProjectedChartScene(const portrayal::PortrayalScene& scene,
                                             const GeoBox& reference_coverage,
                                             const Viewport& viewport) {
  ProjectedChartScene projected;
  projected.has_valid_coverage =
      HasValidCoverage(reference_coverage) && viewport.width > 0 && viewport.height > 0;
  if (!projected.has_valid_coverage) {
    return projected;
  }

  for (const auto& polygon : scene.areas) {
    const bool has_area_overlay = std::any_of(
        polygon.overlays.begin(),
        polygon.overlays.end(),
        [](const portrayal::AreaOverlayStyle& overlay) { return overlay.enabled; });
    if (!polygon.visible ||
        (!polygon.fill.enabled && !polygon.stroke.enabled && !has_area_overlay)) {
      continue;
    }
    if (has_area_overlay) {
      projected.requires_complex_polygon_support = true;
    }

    const auto ring = NormalizeRing(polygon.geometry.outer_ring);
    if (ring.size() < 3) {
      continue;
    }

    if (polygon.fill.enabled) {
      std::vector<std::vector<TriangulationPoint2D>> hole_rings;
      hole_rings.reserve(polygon.geometry.holes.size());
      for (const auto& hole : polygon.geometry.holes) {
        const auto normalized_hole = NormalizeRing(hole);
        if (normalized_hole.size() < 3) {
          continue;
        }

        hole_rings.push_back(
            ProjectRingToPixels(normalized_hole, reference_coverage, viewport));
      }

      TriangulatedPolygon2D triangulated;
      const auto triangulation_status = TriangulatePolygon2D(
          ProjectRingToPixels(ring, reference_coverage, viewport), hole_rings, &triangulated);
      if (!triangulation_status.ok()) {
        projected.requires_complex_polygon_support = true;
        projected.stats.skipped_complex_polygon_count += 1;
        continue;
      }

      if (triangulated.triangle_indices.size() % 3 != 0) {
        projected.requires_complex_polygon_support = true;
        projected.stats.skipped_complex_polygon_count += 1;
        continue;
      }

      std::vector<ProjectedColorVertex> polygon_triangle_vertices;
      polygon_triangle_vertices.reserve(triangulated.triangle_indices.size());
      bool invalid_indices = false;
      for (uint32_t index : triangulated.triangle_indices) {
        if (index >= triangulated.vertices.size()) {
          invalid_indices = true;
          break;
        }

        polygon_triangle_vertices.push_back(MakeColorVertex(
            PixelPoint{
                .x = static_cast<float>(triangulated.vertices[index].x),
                .y = static_cast<float>(triangulated.vertices[index].y),
            },
            polygon.fill.color,
            viewport));
      }
      if (invalid_indices) {
        projected.requires_complex_polygon_support = true;
        projected.stats.skipped_complex_polygon_count += 1;
        continue;
      }
      projected.triangle_vertices.insert(projected.triangle_vertices.end(),
                                         polygon_triangle_vertices.begin(),
                                         polygon_triangle_vertices.end());
    }

    if (polygon.stroke.enabled) {
      if (polygon.stroke.pattern != portrayal::StrokePatternKind::kSolid) {
        projected.requires_complex_line_support = true;
      }
      std::vector<GeoPoint> outline = ring;
      outline.push_back(ring.front());
      AppendPolylineVertices(outline,
                             polygon.stroke.color,
                             reference_coverage,
                             viewport,
                             &projected.line_vertices);
      for (const auto& hole : polygon.geometry.holes) {
        auto hole_outline = NormalizeRing(hole);
        if (hole_outline.size() < 3) {
          continue;
        }
        hole_outline.push_back(hole_outline.front());
        AppendPolylineVertices(hole_outline,
                               polygon.stroke.color,
                               reference_coverage,
                               viewport,
                               &projected.line_vertices);
      }
    }
  }

  for (const auto& polyline : scene.lines) {
    if (!polyline.visible || !polyline.stroke.enabled) {
      continue;
    }
    if (polyline.stroke.pattern != portrayal::StrokePatternKind::kSolid) {
      projected.requires_complex_line_support = true;
    }

    AppendPolylineVertices(polyline.geometry.vertices,
                           polyline.stroke.color,
                           reference_coverage,
                           viewport,
                           &projected.line_vertices);
  }

  for (const auto& point : scene.points) {
    if (!point.visible || !point.symbol.enabled) {
      continue;
    }

    projected.point_vertices.push_back(MakePointVertex(
        ProjectToPixels(point.geometry.position, reference_coverage, viewport),
        point.symbol,
        viewport));
  }

  projected.stats.triangle_vertex_count =
      static_cast<uint64_t>(projected.triangle_vertices.size());
  projected.stats.line_vertex_count =
      static_cast<uint64_t>(projected.line_vertices.size());
  projected.stats.point_vertex_count =
      static_cast<uint64_t>(projected.point_vertices.size());
  return projected;
}

}  // namespace navscene::render
