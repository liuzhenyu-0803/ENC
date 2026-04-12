#include "render/scene_svg_export.h"

#include <iostream>
#include <string_view>

namespace {

bool Expect(bool condition, std::string_view message) {
  if (condition) {
    return true;
  }

  std::cerr << "[navscene-scene-svg] " << message << '\n';
  return false;
}

bool Contains(std::string_view text, std::string_view needle) {
  return text.find(needle) != std::string_view::npos;
}

navscene::render::ChartScene BuildScene() {
  navscene::render::ChartScene scene;
  scene.points.push_back(navscene::render::PointPrimitive{
      .dataset_path = "sample",
      .object_class_code = 129,
      .object_class_acronym = "SOUNDG",
      .position = {.lat = 10.0, .lon = 110.0},
  });
  scene.points.push_back(navscene::render::PointPrimitive{
      .dataset_path = "sample",
      .object_class_code = 300,
      .object_class_acronym = "BOYLAT",
      .position = {.lat = 10.3, .lon = 110.3},
  });
  scene.polylines.push_back(navscene::render::PolylinePrimitive{
      .dataset_path = "sample",
      .object_class_code = 30,
      .object_class_acronym = "COALNE",
      .vertices = {
          {.lat = 9.5, .lon = 109.5},
          {.lat = 10.5, .lon = 110.5},
      },
  });
  scene.polylines.push_back(navscene::render::PolylinePrimitive{
      .dataset_path = "sample",
      .object_class_code = 301,
      .object_class_acronym = "RECTRC",
      .attributes = {{"ORIENT", "45"}},
      .vertices = {
          {.lat = 9.8, .lon = 109.8},
          {.lat = 10.2, .lon = 110.2},
      },
  });
  scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .object_class_code = 42,
      .object_class_acronym = "DEPARE",
      .outer_ring = {
          {.lat = 9.0, .lon = 109.0},
          {.lat = 9.0, .lon = 111.0},
          {.lat = 11.0, .lon = 111.0},
          {.lat = 11.0, .lon = 109.0},
      },
  });
  scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .object_class_code = 302,
      .object_class_acronym = "PRDARE",
      .attributes = {{"OBJNAM", "Refinery Bay"}},
      .outer_ring = {
          {.lat = 9.2, .lon = 109.2},
          {.lat = 9.2, .lon = 109.8},
          {.lat = 9.8, .lon = 109.8},
          {.lat = 9.8, .lon = 109.2},
      },
  });
  scene.stats.point_primitive_count = 2;
  scene.stats.polyline_primitive_count = 2;
  scene.stats.polygon_primitive_count = 2;
  return scene;
}

}  // namespace

int main() {
  const auto svg = navscene::render::ExportChartSceneToSvg(
      BuildScene(),
      navscene::render::SvgExportOptions{
          .width = 640,
          .height = 480,
          .padding = 20,
      });

  if (!Expect(Contains(svg, "<svg"), "SVG export should emit an <svg> root element.")) {
    return 1;
  }
  if (!Expect(Contains(svg, "<path"), "SVG export should emit polygon paths.")) {
    return 1;
  }
  if (!Expect(Contains(svg, "<polyline"), "SVG export should emit line elements.")) {
    return 1;
  }
  if (!Expect(Contains(svg, "<circle"), "SVG export should emit circular point elements.")) {
    return 1;
  }
  if (!Expect(Contains(svg, "<polygon"), "SVG export should emit polygon point elements.")) {
    return 1;
  }
  if (!Expect(Contains(svg, "data-obj=\"DEPARE\""),
              "SVG export should preserve polygon object class metadata.")) {
    return 1;
  }
  if (!Expect(Contains(svg, "data-obj=\"COALNE\""),
              "SVG export should preserve line object class metadata.")) {
    return 1;
  }
  if (!Expect(Contains(svg, "data-obj=\"RECTRC\""),
              "SVG export should preserve dashed route line metadata.")) {
    return 1;
  }
  if (!Expect(Contains(svg, "data-obj=\"SOUNDG\""),
              "SVG export should preserve point object class metadata.")) {
    return 1;
  }
  if (!Expect(Contains(svg, "data-obj=\"BOYLAT\""),
              "SVG export should preserve non-circular point object class metadata.")) {
    return 1;
  }
  if (!Expect(Contains(svg, "stroke-dasharray=\"4,2\""),
              "SVG export should emit stroke-dasharray for dashed portrayal strokes.")) {
    return 1;
  }
  if (!Expect(Contains(svg, "045 deg"),
              "SVG export should include synthesized RECTRC orientation labels.")) {
    return 1;
  }
  if (!Expect(Contains(svg, "Prod Refinery Bay"),
              "SVG export should include prefixed production-area labels.")) {
    return 1;
  }

  return 0;
}
