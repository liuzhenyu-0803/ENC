#include "geo/mercator_projection.h"
#include "render/software_raster.h"

#include "portrayal/engine.h"
#include "render/label_layout.h"

#if defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>

namespace navscene::render {
namespace {

constexpr int kPaddingPixels = 0;

portrayal::DisplaySettings BuildCompatibilitySettings() {
  auto settings = portrayal::MakeDisplaySettings(DisplayOptions{});
  settings.display_category = DisplayCategory::kAll;
  settings.show_meta = true;
  settings.show_quality_of_data = true;
  return settings;
}

bool HasValidCoverage(const GeoBox& coverage) {
  return geo::HasValidCoverage(coverage);
}

POINT ProjectPoint(const GeoPoint& point,
                   const GeoBox& reference_coverage,
                   const Viewport& viewport,
                   int width,
                   int height) {
  Viewport projection_viewport = viewport;
  projection_viewport.width = static_cast<uint32_t>(width);
  projection_viewport.height = static_cast<uint32_t>(height);
  const auto pixel =
      geo::GeoToPixel(reference_coverage, projection_viewport, point, kPaddingPixels);
  POINT projected{};
  projected.x = static_cast<LONG>(std::lround(pixel.first));
  projected.y = static_cast<LONG>(std::lround(pixel.second));
  return projected;
}

COLORREF ToColor(const portrayal::Rgb8& color) {
  return RGB(color.r, color.g, color.b);
}

struct DoublePoint {
  double x = 0.0;
  double y = 0.0;
};

struct ProjectedPolygonGeometry {
  std::vector<POINT> points;
  std::vector<int> counts;
  RECT bounds{
      std::numeric_limits<LONG>::max(),
      std::numeric_limits<LONG>::max(),
      std::numeric_limits<LONG>::min(),
      std::numeric_limits<LONG>::min(),
  };
};

bool HasEnabledOverlay(const portrayal::AreaCommand& polygon) {
  return std::any_of(
      polygon.overlays.begin(),
      polygon.overlays.end(),
      [](const portrayal::AreaOverlayStyle& overlay) { return overlay.enabled; });
}

void ExpandBounds(RECT* bounds, const POINT& point) {
  if (bounds == nullptr) {
    return;
  }
  bounds->left = std::min(bounds->left, point.x);
  bounds->top = std::min(bounds->top, point.y);
  bounds->right = std::max(bounds->right, point.x);
  bounds->bottom = std::max(bounds->bottom, point.y);
}

bool AppendProjectedRing(const std::vector<GeoPoint>& ring,
                         const GeoBox& reference_coverage,
                         const Viewport& viewport,
                         int width,
                         int height,
                         std::vector<POINT>* points,
                         std::vector<int>* counts,
                         RECT* bounds) {
  if (points == nullptr || counts == nullptr || ring.size() < 3) {
    return false;
  }

  counts->push_back(static_cast<int>(ring.size()));
  for (const auto& point : ring) {
    const POINT projected = ProjectPoint(point, reference_coverage, viewport, width, height);
    points->push_back(projected);
    ExpandBounds(bounds, projected);
  }
  return true;
}

ProjectedPolygonGeometry BuildProjectedPolygonGeometry(
    const portrayal::AreaCommand& polygon,
    const GeoBox& reference_coverage,
    const Viewport& viewport,
    int width,
    int height) {
  ProjectedPolygonGeometry projected;
  AppendProjectedRing(polygon.geometry.outer_ring,
                      reference_coverage,
                      viewport,
                      width,
                      height,
                      &projected.points,
                      &projected.counts,
                      &projected.bounds);
  for (const auto& hole : polygon.geometry.holes) {
    AppendProjectedRing(hole,
                        reference_coverage,
                        viewport,
                        width,
                        height,
                        &projected.points,
                        &projected.counts,
                        &projected.bounds);
  }
  return projected;
}

bool HasProjectedBounds(const RECT& bounds) {
  return bounds.left <= bounds.right && bounds.top <= bounds.bottom;
}

void DrawOpenPolyline(HDC hdc, const std::vector<POINT>& points) {
  if (points.size() < 2) {
    return;
  }
  MoveToEx(hdc, points.front().x, points.front().y, nullptr);
  for (size_t index = 1; index < points.size(); ++index) {
    LineTo(hdc, points[index].x, points[index].y);
  }
}

POINT MakePoint(double x, double y) {
  return POINT{
      static_cast<LONG>(std::lround(x)),
      static_cast<LONG>(std::lround(y)),
  };
}

void DrawLocalPolyline(HDC hdc,
                       int origin_x,
                       int origin_y,
                       std::initializer_list<DoublePoint> points) {
  std::vector<POINT> projected;
  projected.reserve(points.size());
  for (const auto& point : points) {
    projected.push_back(
        MakePoint(static_cast<double>(origin_x) + point.x, static_cast<double>(origin_y) + point.y));
  }
  DrawOpenPolyline(hdc, projected);
}

POINT TransformLocalPoint(const POINT& center, double angle_rad, const DoublePoint& local) {
  const double cos_angle = std::cos(angle_rad);
  const double sin_angle = std::sin(angle_rad);
  return MakePoint(static_cast<double>(center.x) + local.x * cos_angle - local.y * sin_angle,
                   static_cast<double>(center.y) + local.x * sin_angle + local.y * cos_angle);
}

void DrawTransformedPolyline(HDC hdc,
                             const POINT& center,
                             double angle_rad,
                             std::initializer_list<DoublePoint> points) {
  std::vector<POINT> projected;
  projected.reserve(points.size());
  for (const auto& point : points) {
    projected.push_back(TransformLocalPoint(center, angle_rad, point));
  }
  DrawOpenPolyline(hdc, projected);
}

void DrawAirportOverlayGlyph(HDC hdc, int origin_x, int origin_y, int spacing_px) {
  const double scale = static_cast<double>(std::max(spacing_px, 48)) / 96.0;
  DrawLocalPolyline(hdc,
                    origin_x,
                    origin_y,
                    {
                        {-11.0 * scale, 3.0 * scale},
                        {-6.0 * scale, 3.0 * scale},
                        {-7.5 * scale, 1.5 * scale},
                        {-7.5 * scale, -0.5 * scale},
                        {-1.5 * scale, -0.5 * scale},
                        {-7.0 * scale, -4.0 * scale},
                        {-7.0 * scale, -7.0 * scale},
                        {-8.5 * scale, -8.5 * scale},
                        {-10.0 * scale, -7.0 * scale},
                        {-10.0 * scale, -4.0 * scale},
                        {-15.0 * scale, -1.0 * scale},
                        {-10.0 * scale, -1.0 * scale},
                        {-10.0 * scale, 2.0 * scale},
                        {-11.0 * scale, 3.0 * scale},
                    });
}

void DrawRockLedgeGlyph(HDC hdc, int origin_x, int origin_y, int spacing_px) {
  const double scale = static_cast<double>(std::max(spacing_px, 56)) / 90.0;
  DrawLocalPolyline(hdc, origin_x, origin_y, {{-12.0 * scale, -4.0 * scale},
                                               {-7.0 * scale, -8.0 * scale}});
  DrawLocalPolyline(hdc, origin_x, origin_y, {{-10.0 * scale, -14.0 * scale},
                                               {-7.0 * scale, -5.0 * scale}});
  DrawLocalPolyline(hdc, origin_x, origin_y, {{0.0 * scale, -11.0 * scale},
                                               {-4.5 * scale, -4.0 * scale}});
  DrawLocalPolyline(hdc, origin_x, origin_y, {{5.0 * scale, -13.0 * scale},
                                               {7.0 * scale, -20.0 * scale}});
  DrawLocalPolyline(hdc, origin_x, origin_y, {{11.0 * scale, -2.0 * scale},
                                               {12.5 * scale, 6.5 * scale}});
  DrawLocalPolyline(hdc, origin_x, origin_y, {{26.0 * scale, -3.0 * scale},
                                               {29.0 * scale, -11.0 * scale}});
  DrawLocalPolyline(hdc, origin_x, origin_y, {{26.0 * scale, -15.0 * scale},
                                               {19.0 * scale, -18.0 * scale}});
  DrawLocalPolyline(hdc, origin_x, origin_y, {{-7.0 * scale, -5.0 * scale},
                                               {-15.0 * scale, -8.5 * scale}});
  DrawLocalPolyline(hdc, origin_x, origin_y, {{4.5 * scale, -3.5 * scale},
                                               {3.5 * scale, -10.5 * scale}});
  DrawLocalPolyline(hdc, origin_x, origin_y, {{-12.0 * scale, 13.0 * scale},
                                               {-11.0 * scale, 5.0 * scale}});
  DrawLocalPolyline(hdc, origin_x, origin_y, {{11.0 * scale, 11.5 * scale},
                                               {17.0 * scale, 6.0 * scale}});
  DrawLocalPolyline(hdc, origin_x, origin_y, {{29.0 * scale, 0.0 * scale},
                                               {21.0 * scale, 3.5 * scale}});
  DrawLocalPolyline(hdc, origin_x, origin_y, {{18.5 * scale, -18.0 * scale},
                                               {24.0 * scale, -22.5 * scale}});
  DrawLocalPolyline(hdc, origin_x, origin_y, {{7.0 * scale, -20.0 * scale},
                                               {9.5 * scale, -12.5 * scale}});
}

void DrawDredgedAreaGlyph(HDC hdc, int origin_x, int origin_y, int spacing_px) {
  const double scale = static_cast<double>(std::max(spacing_px, 40)) / 56.0;
  DrawLocalPolyline(hdc,
                    origin_x,
                    origin_y,
                    {
                        {-8.0 * scale, -8.0 * scale},
                        {8.0 * scale, 8.0 * scale},
                    });
}

void DrawNoDataAreaGlyph(HDC hdc, int origin_x, int origin_y, int spacing_px) {
  const double half_length = static_cast<double>(std::max(spacing_px, 40)) * 0.42;
  DrawLocalPolyline(hdc, origin_x, origin_y, {{-half_length, 0.0}, {half_length, 0.0}});
}

void DrawSurveyReliabilityGlyph(HDC hdc, int origin_x, int origin_y, int spacing_px) {
  const double half_length = static_cast<double>(std::max(spacing_px, 64)) * 0.38;
  DrawLocalPolyline(hdc, origin_x, origin_y, {{-half_length, 0.0}, {half_length, 0.0}});
}

void DrawVegetationWoodedGlyph(HDC hdc, int origin_x, int origin_y, int spacing_px) {
  const double scale = static_cast<double>(std::max(spacing_px, 52)) / 64.0;
  DrawLocalPolyline(hdc,
                    origin_x,
                    origin_y,
                    {
                        {0.0, -12.0 * scale},
                        {0.0, 12.0 * scale},
                    });
  DrawLocalPolyline(hdc,
                    origin_x,
                    origin_y,
                    {
                        {-11.0 * scale, -6.0 * scale},
                        {11.0 * scale, -6.0 * scale},
                    });
  DrawLocalPolyline(hdc,
                    origin_x,
                    origin_y,
                    {
                        {-13.0 * scale, 0.0},
                        {13.0 * scale, 0.0},
                    });
  DrawLocalPolyline(hdc,
                    origin_x,
                    origin_y,
                    {
                        {-10.0 * scale, 6.0 * scale},
                        {10.0 * scale, 6.0 * scale},
                    });
  DrawLocalPolyline(hdc,
                    origin_x,
                    origin_y,
                    {
                        {-7.0 * scale, -12.0 * scale},
                        {7.0 * scale, -12.0 * scale},
                    });
  DrawLocalPolyline(hdc,
                    origin_x,
                    origin_y,
                    {
                        {-7.0 * scale, 12.0 * scale},
                        {7.0 * scale, 12.0 * scale},
                    });
}

void DrawVegetationMangroveGlyph(HDC hdc, int origin_x, int origin_y, int spacing_px) {
  const double scale = static_cast<double>(std::max(spacing_px, 56)) / 72.0;
  const int radius = std::max(static_cast<int>(std::lround(5.0 * scale)), 3);
  Ellipse(hdc,
          origin_x - radius,
          origin_y - radius - static_cast<int>(std::lround(5.0 * scale)),
          origin_x + radius + 1,
          origin_y + radius + 1 - static_cast<int>(std::lround(5.0 * scale)));
  DrawLocalPolyline(hdc,
                    origin_x,
                    origin_y,
                    {
                        {0.0, 8.0 * scale},
                        {0.0, -1.0 * scale},
                    });
  DrawLocalPolyline(hdc,
                    origin_x,
                    origin_y,
                    {
                        {-11.0 * scale, 10.0 * scale},
                        {11.0 * scale, 10.0 * scale},
                    });
}

void DrawQualityOverlayGlyph(HDC hdc,
                             const portrayal::AreaOverlayStyle& overlay,
                             int origin_x,
                             int origin_y) {
  const int left = origin_x - 18;
  const int top = origin_y - 10;
  const int right = origin_x + 18;
  const int bottom = origin_y + 10;
  RoundRect(hdc, left, top, right, bottom, 6, 6);

  auto draw_cross = [&](int x, int y) {
    DrawLocalPolyline(hdc, x, y, {{-3.0, -3.0}, {3.0, 3.0}});
    DrawLocalPolyline(hdc, x, y, {{-3.0, 3.0}, {3.0, -3.0}});
  };
  auto draw_bar = [&](int x, int y, int width) {
    DrawLocalPolyline(hdc,
                      x,
                      y,
                      {{-static_cast<double>(width), 0.0}, {static_cast<double>(width), 0.0}});
  };

  switch (overlay.kind) {
    case portrayal::AreaOverlayKind::kDataQualityA1:
      draw_cross(origin_x - 10, origin_y - 1);
      draw_cross(origin_x, origin_y - 1);
      draw_cross(origin_x + 10, origin_y - 1);
      draw_cross(origin_x - 5, origin_y + 6);
      draw_cross(origin_x + 5, origin_y + 6);
      break;
    case portrayal::AreaOverlayKind::kDataQualityA2:
      draw_cross(origin_x - 10, origin_y - 1);
      draw_cross(origin_x, origin_y - 1);
      draw_cross(origin_x + 10, origin_y - 1);
      draw_cross(origin_x, origin_y + 6);
      break;
    case portrayal::AreaOverlayKind::kDataQualityB:
      draw_cross(origin_x - 10, origin_y - 1);
      draw_cross(origin_x, origin_y - 1);
      draw_cross(origin_x + 10, origin_y - 1);
      break;
    case portrayal::AreaOverlayKind::kDataQualityC:
      draw_bar(origin_x - 10, origin_y - 1, 4);
      draw_bar(origin_x, origin_y - 1, 4);
      draw_bar(origin_x + 10, origin_y - 1, 4);
      DrawLocalPolyline(hdc, origin_x - 13, origin_y + 6, {{0.0, 0.0}, {26.0, 0.0}});
      break;
    case portrayal::AreaOverlayKind::kDataQualityD:
      draw_cross(origin_x - 7, origin_y - 1);
      draw_cross(origin_x + 7, origin_y - 1);
      DrawLocalPolyline(hdc, origin_x - 10, origin_y + 6, {{0.0, 0.0}, {20.0, 0.0}});
      break;
    case portrayal::AreaOverlayKind::kDataQualityUnknown:
      DrawLocalPolyline(hdc,
                        origin_x,
                        origin_y,
                        {{-6.0, -6.0}, {-6.0, 4.0}, {0.0, 8.0}, {6.0, 4.0}, {6.0, -6.0}});
      break;
    case portrayal::AreaOverlayKind::kNoData:
      draw_cross(origin_x - 6, origin_y);
      draw_cross(origin_x + 6, origin_y);
      break;
    case portrayal::AreaOverlayKind::kNoDataArea:
    case portrayal::AreaOverlayKind::kSurveyReliability:
    case portrayal::AreaOverlayKind::kVegetationMangrove:
    case portrayal::AreaOverlayKind::kVegetationWooded:
    default:
      break;
  }
}

void DrawTiledAreaOverlayGlyph(HDC hdc,
                               const portrayal::AreaOverlayStyle& overlay,
                               int origin_x,
                               int origin_y) {
  switch (overlay.kind) {
    case portrayal::AreaOverlayKind::kAirport:
      DrawAirportOverlayGlyph(hdc, origin_x, origin_y, overlay.spacing_px);
      break;
    case portrayal::AreaOverlayKind::kRockLedge:
      DrawRockLedgeGlyph(hdc, origin_x, origin_y, overlay.spacing_px);
      break;
    case portrayal::AreaOverlayKind::kDredgedArea:
      DrawDredgedAreaGlyph(hdc, origin_x, origin_y, overlay.spacing_px);
      break;
    case portrayal::AreaOverlayKind::kDataQualityA1:
    case portrayal::AreaOverlayKind::kDataQualityA2:
    case portrayal::AreaOverlayKind::kDataQualityB:
    case portrayal::AreaOverlayKind::kDataQualityC:
    case portrayal::AreaOverlayKind::kDataQualityD:
    case portrayal::AreaOverlayKind::kDataQualityUnknown:
    case portrayal::AreaOverlayKind::kNoData:
      DrawQualityOverlayGlyph(hdc, overlay, origin_x, origin_y);
      break;
    case portrayal::AreaOverlayKind::kNoDataArea:
      DrawNoDataAreaGlyph(hdc, origin_x, origin_y, overlay.spacing_px);
      break;
    case portrayal::AreaOverlayKind::kSurveyReliability:
      DrawSurveyReliabilityGlyph(hdc, origin_x, origin_y, overlay.spacing_px);
      break;
    case portrayal::AreaOverlayKind::kVegetationMangrove:
      DrawVegetationMangroveGlyph(hdc, origin_x, origin_y, overlay.spacing_px);
      break;
    case portrayal::AreaOverlayKind::kVegetationWooded:
      DrawVegetationWoodedGlyph(hdc, origin_x, origin_y, overlay.spacing_px);
      break;
    default:
      break;
  }
}

void DrawCableBoundaryGlyph(HDC hdc, const POINT& center, double angle_rad) {
  DrawTransformedPolyline(hdc,
                          center,
                          angle_rad,
                          {{-10.0, -9.0}, {-2.0, 4.0}, {8.0, 7.0}, {0.0, 20.0}});
}

void DrawTiledAreaOverlay(HDC hdc,
                          const portrayal::AreaOverlayStyle& overlay,
                          const ProjectedPolygonGeometry& geometry) {
  if (!HasProjectedBounds(geometry.bounds)) {
    return;
  }

  const int saved_dc = SaveDC(hdc);
  BeginPath(hdc);
  PolyPolygon(hdc,
              geometry.points.data(),
              geometry.counts.data(),
              static_cast<int>(geometry.counts.size()));
  EndPath(hdc);
  SelectClipPath(hdc, RGN_AND);

  HPEN pen = CreatePen(PS_SOLID,
                       std::max(overlay.line_width_px, 1),
                       ToColor(overlay.color));
  HGDIOBJ old_pen = SelectObject(hdc, pen);
  HGDIOBJ old_brush = SelectObject(hdc, GetStockObject(NULL_BRUSH));

  const int spacing = std::max(overlay.spacing_px, 36);
  for (int y = geometry.bounds.top - spacing; y <= geometry.bounds.bottom + spacing; y += spacing) {
    for (int x = geometry.bounds.left - spacing; x <= geometry.bounds.right + spacing;
         x += spacing) {
      DrawTiledAreaOverlayGlyph(hdc, overlay, x + spacing / 2, y + spacing / 2);
    }
  }

  SelectObject(hdc, old_brush);
  SelectObject(hdc, old_pen);
  DeleteObject(pen);
  RestoreDC(hdc, saved_dc);
}

std::vector<POINT> BuildProjectedRing(const std::vector<GeoPoint>& ring,
                                      const GeoBox& reference_coverage,
                                      const Viewport& viewport,
                                      int width,
                                      int height) {
  std::vector<POINT> projected_ring;
  if (ring.size() < 2) {
    return projected_ring;
  }

  projected_ring.reserve(ring.size() + 1);
  for (const auto& point : ring) {
    projected_ring.push_back(ProjectPoint(point, reference_coverage, viewport, width, height));
  }
  if (projected_ring.front().x != projected_ring.back().x ||
      projected_ring.front().y != projected_ring.back().y) {
    projected_ring.push_back(projected_ring.front());
  }
  return projected_ring;
}

void DrawOverlayAlongPolyline(HDC hdc,
                              const portrayal::AreaOverlayStyle& overlay,
                              const std::vector<POINT>& ring) {
  if (ring.size() < 2) {
    return;
  }

  const int spacing = std::max(overlay.spacing_px, 48);
  double remaining = static_cast<double>(spacing) * 0.5;
  for (size_t index = 1; index < ring.size(); ++index) {
    const double dx = static_cast<double>(ring[index].x - ring[index - 1].x);
    const double dy = static_cast<double>(ring[index].y - ring[index - 1].y);
    const double length = std::hypot(dx, dy);
    if (length < 1e-3) {
      continue;
    }

    while (remaining <= length) {
      const double t = remaining / length;
      const POINT center = MakePoint(static_cast<double>(ring[index - 1].x) + dx * t,
                                     static_cast<double>(ring[index - 1].y) + dy * t);
      if (overlay.kind == portrayal::AreaOverlayKind::kCableBoundary) {
        DrawCableBoundaryGlyph(hdc, center, std::atan2(dy, dx));
      }
      remaining += static_cast<double>(spacing);
    }
    remaining -= length;
  }
}

void DrawBoundaryAreaOverlay(HDC hdc,
                             const portrayal::AreaCommand& polygon,
                             const portrayal::AreaOverlayStyle& overlay,
                             const GeoBox& reference_coverage,
                             const Viewport& viewport,
                             int width,
                             int height) {
  HPEN pen = CreatePen(PS_SOLID,
                       std::max(overlay.line_width_px, 1),
                       ToColor(overlay.color));
  HGDIOBJ old_pen = SelectObject(hdc, pen);
  HGDIOBJ old_brush = SelectObject(hdc, GetStockObject(NULL_BRUSH));

  DrawOverlayAlongPolyline(hdc,
                           overlay,
                           BuildProjectedRing(
                               polygon.geometry.outer_ring, reference_coverage, viewport, width, height));
  for (const auto& hole : polygon.geometry.holes) {
    DrawOverlayAlongPolyline(hdc,
                             overlay,
                             BuildProjectedRing(hole, reference_coverage, viewport, width, height));
  }

  SelectObject(hdc, old_brush);
  SelectObject(hdc, old_pen);
  DeleteObject(pen);
}

void DrawAreaOverlays(HDC hdc,
                      const portrayal::AreaCommand& polygon,
                      const ProjectedPolygonGeometry& geometry,
                      const GeoBox& reference_coverage,
                      const Viewport& viewport,
                      int width,
                      int height) {
  for (const auto& overlay : polygon.overlays) {
    if (!overlay.enabled) {
      continue;
    }
    if (overlay.placement == portrayal::AreaOverlayPlacement::kAlongBoundary) {
      DrawBoundaryAreaOverlay(hdc, polygon, overlay, reference_coverage, viewport, width, height);
      continue;
    }
    DrawTiledAreaOverlay(hdc, overlay, geometry);
  }
}

HPEN CreateStrokePen(const portrayal::StrokeStyle& style) {
  const int width = std::max(style.width_px, 1);
  if (style.pattern == portrayal::StrokePatternKind::kSolid) {
    return CreatePen(PS_SOLID, width, ToColor(style.color));
  }

  LOGBRUSH brush{};
  brush.lbStyle = BS_SOLID;
  brush.lbColor = ToColor(style.color);

  std::vector<DWORD> pattern;
  switch (style.pattern) {
    case portrayal::StrokePatternKind::kDash:
      pattern = {
          static_cast<DWORD>(std::max(width * 4, 2)),
          static_cast<DWORD>(std::max(width * 2, 2)),
      };
      break;
    case portrayal::StrokePatternKind::kDot:
      pattern = {
          static_cast<DWORD>(std::max(width, 1)),
          static_cast<DWORD>(std::max(width * 2, 2)),
      };
      break;
    case portrayal::StrokePatternKind::kSolid:
    default:
      break;
  }

  if (pattern.empty()) {
    return CreatePen(PS_SOLID, width, ToColor(style.color));
  }

  return ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE | PS_ENDCAP_FLAT | PS_JOIN_MITER,
                      static_cast<DWORD>(width),
                      &brush,
                      static_cast<DWORD>(pattern.size()),
                      pattern.data());
}

std::vector<POINT> BuildPointSymbolVertices(const POINT& center,
                                            const portrayal::PointSymbolStyle& style) {
  const int radius = std::max(style.size_px / 2, 1);
  switch (style.kind) {
    case portrayal::PointSymbolKind::kTriangle:
      return {
          POINT{center.x, center.y - radius},
          POINT{center.x - radius, center.y + radius},
          POINT{center.x + radius, center.y + radius},
      };
    case portrayal::PointSymbolKind::kSquare:
      return {
          POINT{center.x - radius, center.y - radius},
          POINT{center.x + radius, center.y - radius},
          POINT{center.x + radius, center.y + radius},
          POINT{center.x - radius, center.y + radius},
      };
    case portrayal::PointSymbolKind::kDiamond:
      return {
          POINT{center.x, center.y - radius},
          POINT{center.x + radius, center.y},
          POINT{center.x, center.y + radius},
          POINT{center.x - radius, center.y},
      };
    case portrayal::PointSymbolKind::kCircle:
    default:
      return {};
  }
}

void FillBackground(HDC hdc, const RECT& rect, const portrayal::Rgb8& color) {
  HBRUSH brush = CreateSolidBrush(ToColor(color));
  FillRect(hdc, &rect, brush);
  DeleteObject(brush);
}

void DrawPolygons(HDC hdc,
                  const std::vector<portrayal::AreaCommand>& polygons,
                  const GeoBox& reference_coverage,
                  const Viewport& viewport,
                  int width,
                  int height) {
  HGDIOBJ old_pen = SelectObject(hdc, GetStockObject(NULL_PEN));
  HGDIOBJ old_brush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
  SetPolyFillMode(hdc, ALTERNATE);

  for (const auto& polygon : polygons) {
    if (!polygon.visible ||
        (!polygon.fill.enabled && !polygon.stroke.enabled && !HasEnabledOverlay(polygon))) {
      continue;
    }

    HPEN polygon_pen = polygon.stroke.enabled ? CreateStrokePen(polygon.stroke) : nullptr;
    HBRUSH polygon_brush =
        polygon.fill.enabled ? CreateSolidBrush(ToColor(polygon.fill.color)) : nullptr;
    SelectObject(hdc,
                 polygon_pen != nullptr ? static_cast<HGDIOBJ>(polygon_pen)
                                        : GetStockObject(NULL_PEN));
    SelectObject(hdc,
                 polygon_brush != nullptr ? static_cast<HGDIOBJ>(polygon_brush)
                                          : GetStockObject(NULL_BRUSH));

    const ProjectedPolygonGeometry projected = BuildProjectedPolygonGeometry(
        polygon, reference_coverage, viewport, width, height);

    if (!projected.counts.empty() && !projected.points.empty()) {
      PolyPolygon(hdc,
                  projected.points.data(),
                  projected.counts.data(),
                  static_cast<int>(projected.counts.size()));
      if (HasEnabledOverlay(polygon)) {
        DrawAreaOverlays(
            hdc, polygon, projected, reference_coverage, viewport, width, height);
      }
    }

    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    SelectObject(hdc, GetStockObject(NULL_PEN));
    if (polygon_brush != nullptr) {
      DeleteObject(polygon_brush);
    }
    if (polygon_pen != nullptr) {
      DeleteObject(polygon_pen);
    }
  }

  SelectObject(hdc, old_brush);
  SelectObject(hdc, old_pen);
}

void DrawPolylines(HDC hdc,
                   const std::vector<portrayal::LineCommand>& polylines,
                   const GeoBox& reference_coverage,
                   const Viewport& viewport,
                   int width,
                   int height) {
  HPEN pen = CreateStrokePen(portrayal::StrokeStyle{
      .color = {238, 197, 93},
      .width_px = 1,
      .pattern = portrayal::StrokePatternKind::kSolid,
      .enabled = true,
  });
  HGDIOBJ old_pen = SelectObject(hdc, pen);

  for (const auto& polyline : polylines) {
    if (!polyline.visible || !polyline.stroke.enabled) {
      continue;
    }

    SelectObject(hdc, old_pen);
    DeleteObject(pen);
    pen = CreateStrokePen(polyline.stroke);
    old_pen = SelectObject(hdc, pen);

    if (polyline.geometry.vertices.size() < 2) {
      continue;
    }

    std::vector<POINT> points;
    points.reserve(polyline.geometry.vertices.size());
    for (const auto& point : polyline.geometry.vertices) {
      points.push_back(ProjectPoint(point, reference_coverage, viewport, width, height));
    }
    Polyline(hdc, points.data(), static_cast<int>(points.size()));
  }

  SelectObject(hdc, old_pen);
  DeleteObject(pen);
}

void DrawPoints(HDC hdc,
                const std::vector<portrayal::PointCommand>& points,
                const GeoBox& reference_coverage,
                const Viewport& viewport,
                int width,
                int height) {
  HPEN pen = CreatePen(PS_SOLID, 1, RGB(232, 245, 255));
  HBRUSH brush = CreateSolidBrush(RGB(134, 215, 255));
  HGDIOBJ old_pen = SelectObject(hdc, pen);
  HGDIOBJ old_brush = SelectObject(hdc, brush);

  for (const auto& point : points) {
    if (!point.visible || !point.symbol.enabled) {
      continue;
    }

    SelectObject(hdc, old_brush);
    SelectObject(hdc, old_pen);
    DeleteObject(brush);
    DeleteObject(pen);
    pen = CreatePen(PS_SOLID, 1, ToColor(point.symbol.stroke));
    brush = CreateSolidBrush(ToColor(point.symbol.fill));
    old_pen = SelectObject(hdc, pen);
    old_brush = SelectObject(hdc, brush);

    const POINT p =
        ProjectPoint(point.geometry.position, reference_coverage, viewport, width, height);
    const int radius = std::max(point.symbol.size_px / 2, 1);
    const auto vertices = BuildPointSymbolVertices(p, point.symbol);
    if (!vertices.empty()) {
      Polygon(hdc, vertices.data(), static_cast<int>(vertices.size()));
      continue;
    }
    Ellipse(hdc,
            p.x - radius,
            p.y - radius,
            p.x + radius + 1,
            p.y + radius + 1);
  }

  SelectObject(hdc, old_brush);
  SelectObject(hdc, old_pen);
  DeleteObject(brush);
  DeleteObject(pen);
}

HFONT CreateLabelFont(HDC hdc, const portrayal::TextStyle& style) {
  const int logical_height = -std::max(
      MulDiv(std::max(style.size_px, 8), GetDeviceCaps(hdc, LOGPIXELSY), 96),
      8);
  const int weight =
      style.role == portrayal::FontRole::kImportant ? FW_SEMIBOLD : FW_NORMAL;
  const char* face_name =
      style.role == portrayal::FontRole::kSounding ? "Consolas" : "Segoe UI";
  return CreateFontA(logical_height,
                     0,
                     0,
                     0,
                     weight,
                     FALSE,
                     FALSE,
                     FALSE,
                     DEFAULT_CHARSET,
                     OUT_DEFAULT_PRECIS,
                     CLIP_DEFAULT_PRECIS,
                     CLEARTYPE_QUALITY,
                     DEFAULT_PITCH | FF_DONTCARE,
                     face_name);
}

void DrawLabels(HDC hdc,
                const portrayal::PortrayalScene& scene,
                const GeoBox& reference_coverage,
                const Viewport& viewport,
                int width,
                int height) {
  const auto labels = LayoutPortrayalLabels(
      scene,
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height),
      [&](const GeoPoint& point) {
        const POINT projected =
            ProjectPoint(point, reference_coverage, viewport, width, height);
        return ScreenPoint{
            .x = static_cast<double>(projected.x),
            .y = static_cast<double>(projected.y),
        };
      });

  SetBkMode(hdc, TRANSPARENT);
  for (const auto& label : labels) {
    HFONT font = CreateLabelFont(hdc, label.style);
    HGDIOBJ old_font = SelectObject(hdc, font);

    if (label.style.halo) {
      SetTextColor(hdc, RGB(255, 255, 255));
      static constexpr POINT kHaloOffsets[] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
      for (const auto& offset : kHaloOffsets) {
        TextOutA(hdc,
                 static_cast<int>(label.origin.x) + offset.x,
                 static_cast<int>(label.origin.y) + offset.y,
                 label.text.c_str(),
                 static_cast<int>(label.text.size()));
      }
    }

    SetTextColor(hdc, ToColor(label.style.color));
    TextOutA(hdc,
             static_cast<int>(label.origin.x),
             static_cast<int>(label.origin.y),
             label.text.c_str(),
             static_cast<int>(label.text.size()));

    SelectObject(hdc, old_font);
    DeleteObject(font);
  }
}

void RenderToHdc(HDC hdc,
                 int width,
                 int height,
                 const portrayal::PortrayalScene& scene,
                 const GeoBox& reference_coverage,
                 const Viewport& viewport) {
  const RECT rect{0, 0, width, height};
  FillBackground(hdc, rect, scene.background_color);

  if (HasValidCoverage(reference_coverage)) {
    DrawPolygons(hdc, scene.areas, reference_coverage, viewport, width, height);
    DrawPolylines(hdc, scene.lines, reference_coverage, viewport, width, height);
    DrawPoints(hdc, scene.points, reference_coverage, viewport, width, height);
    DrawLabels(hdc, scene, reference_coverage, viewport, width, height);
  }
}

BITMAPINFO BuildBitmapInfo(uint32_t width, uint32_t height) {
  BITMAPINFO info{};
  info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  info.bmiHeader.biWidth = static_cast<LONG>(width);
  info.bmiHeader.biHeight = -static_cast<LONG>(height);
  info.bmiHeader.biPlanes = 1;
  info.bmiHeader.biBitCount = 32;
  info.bmiHeader.biCompression = BI_RGB;
  return info;
}

}  // namespace

Status RasterizeChartSceneWin32(const ChartScene& scene,
                                const GeoBox& reference_coverage,
                                const Viewport& viewport,
                                uint32_t width,
                                uint32_t height,
                                SoftwareRasterImage* out) {
  return RasterizeChartSceneWin32(
      portrayal::BuildPortrayalScene(scene, BuildCompatibilitySettings()),
      reference_coverage,
      viewport,
      width,
      height,
      out);
}

Status RasterizeChartSceneWin32(const portrayal::PortrayalScene& scene,
                                const GeoBox& reference_coverage,
                                const Viewport& viewport,
                                uint32_t width,
                                uint32_t height,
                                SoftwareRasterImage* out) {
  if (out == nullptr) {
    return Status{StatusCode::kInvalidArgument,
                  "Software raster output image must not be null."};
  }

  out->width = width;
  out->height = height;
  out->bgra_pixels.clear();

  if (width == 0 || height == 0) {
    return {};
  }

  HDC screen_dc = GetDC(nullptr);
  if (screen_dc == nullptr) {
    return Status{StatusCode::kIoError, "GetDC failed for software rasterization."};
  }

  HDC memory_dc = CreateCompatibleDC(screen_dc);
  if (memory_dc == nullptr) {
    ReleaseDC(nullptr, screen_dc);
    return Status{StatusCode::kIoError, "CreateCompatibleDC failed for software rasterization."};
  }

  void* bits = nullptr;
  const BITMAPINFO bitmap_info = BuildBitmapInfo(width, height);
  HBITMAP bitmap =
      CreateDIBSection(memory_dc, &bitmap_info, DIB_RGB_COLORS, &bits, nullptr, 0);
  if (bitmap == nullptr || bits == nullptr) {
    if (bitmap != nullptr) {
      DeleteObject(bitmap);
    }
    DeleteDC(memory_dc);
    ReleaseDC(nullptr, screen_dc);
    return Status{StatusCode::kIoError, "CreateDIBSection failed for software rasterization."};
  }

  HGDIOBJ old_bitmap = SelectObject(memory_dc, bitmap);
  RenderToHdc(memory_dc,
              static_cast<int>(width),
              static_cast<int>(height),
              scene,
              reference_coverage,
              viewport);

  const size_t byte_count =
      static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
  out->bgra_pixels.resize(byte_count);
  std::memcpy(out->bgra_pixels.data(), bits, byte_count);

  SelectObject(memory_dc, old_bitmap);
  DeleteObject(bitmap);
  DeleteDC(memory_dc);
  ReleaseDC(nullptr, screen_dc);
  return {};
}

Status PresentSoftwareRasterWin32(const SoftwareRasterImage& image,
                                  const NativeSurfaceDesc& surface) {
  if (surface.type != SurfaceType::kWindow || surface.window_handle == nullptr) {
    return Status{StatusCode::kInvalidArgument,
                  "Software raster presentation requires a native window handle."};
  }
  if (surface.platform != NativePlatform::kWin32) {
    return Status{StatusCode::kUnsupported,
                  "Software raster presentation only supports Win32 surfaces."};
  }
  if (image.width == 0 || image.height == 0 || image.bgra_pixels.empty()) {
    return {};
  }

  HWND hwnd = static_cast<HWND>(surface.window_handle);
  RECT rect{};
  if (!GetClientRect(hwnd, &rect)) {
    return Status{StatusCode::kIoError, "GetClientRect failed for software raster presentation."};
  }

  HDC hdc = GetDC(hwnd);
  if (hdc == nullptr) {
    return Status{StatusCode::kIoError, "GetDC failed for software raster presentation."};
  }

  BITMAPINFO bitmap_info = BuildBitmapInfo(image.width, image.height);
  const int dest_width = std::max(static_cast<int>(rect.right - rect.left), 0);
  const int dest_height = std::max(static_cast<int>(rect.bottom - rect.top), 0);
  if (dest_width > 0 && dest_height > 0) {
    StretchDIBits(hdc,
                  0,
                  0,
                  dest_width,
                  dest_height,
                  0,
                  0,
                  static_cast<int>(image.width),
                  static_cast<int>(image.height),
                  image.bgra_pixels.data(),
                  &bitmap_info,
                  DIB_RGB_COLORS,
                  SRCCOPY);
  }

  ReleaseDC(hwnd, hdc);
  return {};
}

}  // namespace navscene::render

#else

namespace navscene::render {

Status RasterizeChartSceneWin32(const ChartScene&,
                                const GeoBox&,
                                const Viewport&,
                                uint32_t,
                                uint32_t,
                                SoftwareRasterImage*) {
  return Status{StatusCode::kUnsupported,
                "Win32 software rasterization is only available on Windows."};
}

Status PresentSoftwareRasterWin32(const SoftwareRasterImage&,
                                  const NativeSurfaceDesc&) {
  return Status{StatusCode::kUnsupported,
                "Win32 software raster presentation is only available on Windows."};
}

}  // namespace navscene::render

#endif
