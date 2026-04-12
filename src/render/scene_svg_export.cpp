#include "render/scene_svg_export.h"

#include "portrayal/engine.h"
#include "render/label_layout.h"
#include "render/scene_signature.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

namespace navscene::render {
namespace {

portrayal::DisplaySettings BuildCompatibilitySettings() {
  auto settings = portrayal::MakeDisplaySettings(DisplayOptions{});
  settings.display_category = DisplayCategory::kAll;
  settings.show_meta = true;
  settings.show_quality_of_data = true;
  return settings;
}

struct SvgPoint {
  double x = 0.0;
  double y = 0.0;
};

std::string ToHexColor(const portrayal::Rgb8& color) {
  constexpr char kHex[] = "0123456789ABCDEF";
  std::string value = "#000000";
  value[1] = kHex[(color.r >> 4) & 0xF];
  value[2] = kHex[color.r & 0xF];
  value[3] = kHex[(color.g >> 4) & 0xF];
  value[4] = kHex[color.g & 0xF];
  value[5] = kHex[(color.b >> 4) & 0xF];
  value[6] = kHex[color.b & 0xF];
  return value;
}

SvgPoint ProjectPoint(const GeoPoint& point,
                      const GeoBox& coverage,
                      const SvgExportOptions& options) {
  const double world_width = std::max(coverage.max_lon - coverage.min_lon, 1e-9);
  const double world_height = std::max(coverage.max_lat - coverage.min_lat, 1e-9);
  const double draw_width =
      std::max(static_cast<double>(options.width) - options.padding * 2.0, 1.0);
  const double draw_height =
      std::max(static_cast<double>(options.height) - options.padding * 2.0, 1.0);
  const double scale =
      std::min(draw_width / world_width, draw_height / world_height);
  const double origin_x =
      static_cast<double>(options.padding) + (draw_width - world_width * scale) * 0.5;
  const double origin_y =
      static_cast<double>(options.padding) + (draw_height - world_height * scale) * 0.5;

  return SvgPoint{
      .x = origin_x + (point.lon - coverage.min_lon) * scale,
      .y = origin_y + (coverage.max_lat - point.lat) * scale,
  };
}

template <typename Ring>
void AppendSvgPathRing(std::ostringstream& stream,
                       const Ring& ring,
                       const GeoBox& coverage,
                       const SvgExportOptions& options) {
  if (ring.empty()) {
    return;
  }

  const auto first = ProjectPoint(ring.front(), coverage, options);
  stream << "M " << first.x << ' ' << first.y;
  for (size_t index = 1; index < ring.size(); ++index) {
    const auto point = ProjectPoint(ring[index], coverage, options);
    stream << " L " << point.x << ' ' << point.y;
  }
  stream << " Z ";
}

std::string EscapeXml(std::string_view text) {
  std::string escaped;
  escaped.reserve(text.size());
  for (const char ch : text) {
    switch (ch) {
      case '&':
        escaped += "&amp;";
        break;
      case '<':
        escaped += "&lt;";
        break;
      case '>':
        escaped += "&gt;";
        break;
      case '"':
        escaped += "&quot;";
        break;
      default:
        escaped.push_back(ch);
        break;
    }
  }
  return escaped;
}

std::string FontFamilyFor(const portrayal::TextStyle& style) {
  return style.role == portrayal::FontRole::kSounding ? "Consolas" : "Segoe UI";
}

std::string StrokeDashArrayFor(const portrayal::StrokeStyle& style) {
  const int width = std::max(style.width_px, 1);
  switch (style.pattern) {
    case portrayal::StrokePatternKind::kDash:
      return std::to_string(width * 4) + "," + std::to_string(width * 2);
    case portrayal::StrokePatternKind::kDot:
      return std::to_string(width) + "," + std::to_string(width * 2);
    case portrayal::StrokePatternKind::kSolid:
    default:
      return {};
  }
}

void AppendStrokeAttributes(std::ostringstream& stream, const portrayal::StrokeStyle& style) {
  const std::string dash_array = StrokeDashArrayFor(style);
  if (!dash_array.empty()) {
    stream << " stroke-dasharray=\"" << dash_array << "\"";
  }
}

std::string SvgPolygonPointsForSymbol(const SvgPoint& center,
                                      const portrayal::PointSymbolStyle& style) {
  const double radius = std::max(style.size_px / 2, 1);
  std::ostringstream stream;
  stream.setf(std::ios::fixed);
  stream.precision(2);
  switch (style.kind) {
    case portrayal::PointSymbolKind::kTriangle:
      stream << center.x << ',' << center.y - radius << ' '
             << center.x - radius << ',' << center.y + radius << ' '
             << center.x + radius << ',' << center.y + radius;
      return stream.str();
    case portrayal::PointSymbolKind::kDiamond:
      stream << center.x << ',' << center.y - radius << ' '
             << center.x + radius << ',' << center.y << ' '
             << center.x << ',' << center.y + radius << ' '
             << center.x - radius << ',' << center.y;
      return stream.str();
    default:
      return {};
  }
}

}  // namespace

std::string ExportChartSceneToSvg(const ChartScene& scene, const SvgExportOptions& options) {
  return ExportChartSceneToSvg(
      portrayal::BuildPortrayalScene(scene, BuildCompatibilitySettings()),
      options);
}

std::string ExportChartSceneToSvg(const portrayal::PortrayalScene& scene,
                                  const SvgExportOptions& options) {
  render::ChartScene source_scene;
  source_scene.points.reserve(scene.points.size());
  source_scene.polylines.reserve(scene.lines.size());
  source_scene.polygons.reserve(scene.areas.size());
  for (const auto& point : scene.points) {
    source_scene.points.push_back(point.geometry);
  }
  for (const auto& line : scene.lines) {
    source_scene.polylines.push_back(line.geometry);
  }
  for (const auto& area : scene.areas) {
    source_scene.polygons.push_back(area.geometry);
  }
  const auto signature = BuildChartSceneSignature(source_scene);

  std::ostringstream stream;
  stream.setf(std::ios::fixed);
  stream.precision(2);

  stream << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << options.width
         << "\" height=\"" << options.height << "\" viewBox=\"0 0 " << options.width << ' '
         << options.height << "\">";
  stream << "<rect width=\"100%\" height=\"100%\" fill=\""
         << ToHexColor(scene.background_color) << "\"/>";

  if (!signature.has_coverage) {
    stream << "</svg>";
    return stream.str();
  }

  for (const auto& polygon : scene.areas) {
    if (polygon.geometry.outer_ring.size() < 3 || !polygon.visible ||
        (!polygon.fill.enabled && !polygon.stroke.enabled)) {
      continue;
    }

    std::ostringstream path;
    path.setf(std::ios::fixed);
    path.precision(2);
    AppendSvgPathRing(path, polygon.geometry.outer_ring, signature.coverage, options);
    for (const auto& hole : polygon.geometry.holes) {
      if (hole.size() >= 3) {
        AppendSvgPathRing(path, hole, signature.coverage, options);
      }
    }

    stream << "<path fill=\""
           << (polygon.fill.enabled ? ToHexColor(polygon.fill.color) : std::string("none"))
           << "\" stroke=\""
           << (polygon.stroke.enabled ? ToHexColor(polygon.stroke.color) : std::string("none"))
           << "\" stroke-width=\"" << std::max(polygon.stroke.width_px, 1)
           << "\"";
    AppendStrokeAttributes(stream, polygon.stroke);
    stream << " fill-rule=\"evenodd\" data-obj=\""
           << polygon.geometry.object_class_acronym << "\" d=\"" << path.str() << "\"/>";
  }

  for (const auto& polyline : scene.lines) {
    if (polyline.geometry.vertices.size() < 2 || !polyline.visible || !polyline.stroke.enabled) {
      continue;
    }

    stream << "<polyline fill=\"none\" stroke=\"" << ToHexColor(polyline.stroke.color)
           << "\" stroke-width=\"" << polyline.stroke.width_px << "\"";
    AppendStrokeAttributes(stream, polyline.stroke);
    stream << " data-obj=\""
           << polyline.geometry.object_class_acronym << "\" points=\"";
    for (size_t index = 0; index < polyline.geometry.vertices.size(); ++index) {
      const auto point = ProjectPoint(polyline.geometry.vertices[index], signature.coverage, options);
      if (index > 0) {
        stream << ' ';
      }
      stream << point.x << ',' << point.y;
    }
    stream << "\"/>";
  }

  for (const auto& point : scene.points) {
    if (!point.visible || !point.symbol.enabled) {
      continue;
    }

    const auto projected = ProjectPoint(point.geometry.position, signature.coverage, options);
    const int radius = std::max(point.symbol.size_px / 2, 1);
    if (point.symbol.kind == portrayal::PointSymbolKind::kSquare) {
      stream << "<rect x=\"" << projected.x - radius << "\" y=\"" << projected.y - radius
             << "\" width=\"" << radius * 2 << "\" height=\"" << radius * 2
             << "\" fill=\"" << ToHexColor(point.symbol.fill) << "\" stroke=\""
             << ToHexColor(point.symbol.stroke)
             << "\" stroke-width=\"1\" data-obj=\"" << point.geometry.object_class_acronym
             << "\"/>";
      continue;
    }

    const std::string polygon_points = SvgPolygonPointsForSymbol(projected, point.symbol);
    if (!polygon_points.empty()) {
      stream << "<polygon points=\"" << polygon_points << "\" fill=\""
             << ToHexColor(point.symbol.fill) << "\" stroke=\""
             << ToHexColor(point.symbol.stroke)
             << "\" stroke-width=\"1\" data-obj=\"" << point.geometry.object_class_acronym
             << "\"/>";
      continue;
    }

    stream << "<circle cx=\"" << projected.x << "\" cy=\"" << projected.y
           << "\" r=\"" << radius << "\" fill=\"" << ToHexColor(point.symbol.fill)
           << "\" stroke=\"" << ToHexColor(point.symbol.stroke)
           << "\" stroke-width=\"1\" data-obj=\"" << point.geometry.object_class_acronym
           << "\"/>";
  }

  const auto placed_labels = LayoutPortrayalLabels(
      scene,
      options.width,
      options.height,
      [&](const GeoPoint& point) {
        const auto projected = ProjectPoint(point, signature.coverage, options);
        return ScreenPoint{.x = projected.x, .y = projected.y};
      });
  for (const auto& label : placed_labels) {
    if (label.style.halo) {
      stream << "<text x=\"" << label.origin.x << "\" y=\"" << label.origin.y
             << "\" font-size=\"" << label.style.size_px
             << "\" font-family=\"" << FontFamilyFor(label.style)
             << "\" fill=\"#FFFFFF\" text-anchor=\"start\">"
             << EscapeXml(label.text) << "</text>";
    }
    stream << "<text x=\"" << label.origin.x << "\" y=\"" << label.origin.y
           << "\" font-size=\"" << label.style.size_px
           << "\" font-family=\"" << FontFamilyFor(label.style)
           << "\" fill=\"" << ToHexColor(label.style.color)
           << "\" text-anchor=\"start\">" << EscapeXml(label.text) << "</text>";
  }

  stream << "</svg>";
  return stream.str();
}

}  // namespace navscene::render
