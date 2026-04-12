#include "render/portrayal.h"
#include "portrayal/engine.h"

#include <iostream>
#include <string_view>

namespace {

bool Expect(bool condition, std::string_view message) {
  if (condition) {
    return true;
  }

  std::cerr << "[navscene-portrayal-mvp] " << message << '\n';
  return false;
}

navscene::render::PolygonPrimitive MakeArea(std::string_view acronym) {
  navscene::render::PolygonPrimitive primitive;
  primitive.object_class_acronym = std::string(acronym);
  return primitive;
}

navscene::render::PolylinePrimitive MakeLine(std::string_view acronym) {
  navscene::render::PolylinePrimitive primitive;
  primitive.object_class_acronym = std::string(acronym);
  return primitive;
}

navscene::render::PointPrimitive MakePoint(std::string_view acronym) {
  navscene::render::PointPrimitive primitive;
  primitive.object_class_acronym = std::string(acronym);
  return primitive;
}

navscene::render::ChartScene BuildFilterScene() {
  navscene::render::ChartScene scene;
  scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 1,
      .object_class_acronym = "DEPARE",
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 2,
      .object_class_acronym = "M_COVR",
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  scene.points.push_back(navscene::render::PointPrimitive{
      .dataset_path = "sample",
      .feature_id = 3,
      .object_class_acronym = "SOUNDG",
      .attributes = {{"VALSOU", "12.4"}},
      .position = {.lat = 0.5, .lon = 0.5},
  });
  scene.points.push_back(navscene::render::PointPrimitive{
      .dataset_path = "sample",
      .feature_id = 4,
      .object_class_acronym = "BOYLAT",
      .position = {.lat = 0.75, .lon = 0.75},
  });
  return scene;
}

}  // namespace

int main() {
  const auto depare = navscene::render::ResolveAreaPaintStyle(MakeArea("DEPARE"));
  if (!Expect(depare.visible, "DEPARE area should be visible.")) {
    return 1;
  }
  if (!Expect(depare.fill.r == 186 && depare.fill.g == 213 && depare.fill.b == 225,
              "DEPARE area should use the medium-depth water fill palette.")) {
    return 1;
  }

  const auto seaare = navscene::render::ResolveAreaPaintStyle(MakeArea("SEAARE"));
  if (!Expect(!seaare.visible,
              "SEAARE area should not force a generic solid fill.")) {
    return 1;
  }

  const auto coverage = navscene::render::ResolveAreaPaintStyle(MakeArea("M_COVR"));
  if (!Expect(coverage.visible && coverage.stroke.r == 125 && coverage.stroke.g == 137 &&
                  coverage.stroke.b == 140,
              "M_COVR area should use the metadata stroke palette.")) {
    return 1;
  }

  const auto coastline = navscene::render::ResolveLinePaintStyle(MakeLine("COALNE"));
  if (!Expect(coastline.visible && coastline.width == 2,
              "COALNE line should be visible with emphasized width.")) {
    return 1;
  }
  if (!Expect(coastline.stroke.r == 82 && coastline.stroke.g == 90 &&
                  coastline.stroke.b == 92,
              "COALNE line should use the coastline palette.")) {
    return 1;
  }

  const auto depth_contour = navscene::render::ResolveLinePaintStyle(MakeLine("DEPCNT"));
  if (!Expect(depth_contour.stroke.r == 125 && depth_contour.stroke.g == 137 &&
                  depth_contour.stroke.b == 140,
              "DEPCNT line should keep the S-52 contour palette.")) {
    return 1;
  }

  const auto route_centerline = navscene::render::ResolveLinePaintStyle(MakeLine("RECTRC"));
  if (!Expect(route_centerline.visible &&
                  route_centerline.pattern == navscene::portrayal::StrokePatternKind::kDash,
              "RECTRC line should use the dashed route-centerline style.")) {
    return 1;
  }

  const auto pipeline = navscene::render::ResolveLinePaintStyle(MakeLine("PIPSOL"));
  if (!Expect(pipeline.visible &&
                  pipeline.pattern == navscene::portrayal::StrokePatternKind::kDash,
              "PIPSOL line should use a dashed pipeline style.")) {
    return 1;
  }

  const auto land_elevation = navscene::render::ResolveLinePaintStyle(MakeLine("LNDELV"));
  if (!Expect(land_elevation.visible && land_elevation.stroke.r == 139 &&
                  land_elevation.stroke.g == 102 && land_elevation.stroke.b == 31,
              "LNDELV should use the dedicated land-elevation contour stroke.")) {
    return 1;
  }

  const auto sounding = navscene::render::ResolvePointPaintStyle(MakePoint("SOUNDG"));
  if (!Expect(sounding.visible && sounding.radius == 2,
              "SOUNDG point should use the sounding marker size.")) {
    return 1;
  }
  if (!Expect(sounding.fill.r == 125 && sounding.fill.g == 137 && sounding.fill.b == 140,
              "SOUNDG point should use the S-52 sounding palette.")) {
    return 1;
  }

  const auto buoy = navscene::render::ResolvePointPaintStyle(MakePoint("BOYLAT"));
  if (!Expect(buoy.kind == navscene::portrayal::PointSymbolKind::kTriangle &&
                  buoy.radius == 3 && buoy.fill.r == 244,
              "Buoy point should use the emphasized navigation marker style.")) {
    return 1;
  }

  const auto landmark = navscene::render::ResolvePointPaintStyle(MakePoint("LNDMRK"));
  if (!Expect(landmark.kind == navscene::portrayal::PointSymbolKind::kDiamond &&
                  landmark.visible && landmark.fill.r == 177,
              "LNDMRK should use the dedicated landmark point style.")) {
    return 1;
  }

  const auto special_buoy = navscene::render::ResolvePointPaintStyle(MakePoint("BOYSPP"));
  if (!Expect(special_buoy.kind == navscene::portrayal::PointSymbolKind::kDiamond &&
                  special_buoy.visible && special_buoy.stroke.r == 197,
              "BOYSPP should use the dedicated special-purpose buoy style.")) {
    return 1;
  }

  const auto cardinal_beacon = navscene::render::ResolvePointPaintStyle(MakePoint("BCNCAR"));
  if (!Expect(cardinal_beacon.kind == navscene::portrayal::PointSymbolKind::kDiamond &&
                  cardinal_beacon.visible && cardinal_beacon.fill.r == 244,
              "BCNCAR should use the dedicated cardinal beacon style.")) {
    return 1;
  }

  const auto rock = navscene::render::ResolvePointPaintStyle(MakePoint("UWTROC"));
  if (!Expect(rock.kind == navscene::portrayal::PointSymbolKind::kDiamond &&
                  rock.visible && rock.fill.r == 152,
              "UWTROC should use the dedicated underwater rock style.")) {
    return 1;
  }

  const auto fallback_area = navscene::render::ResolveAreaPaintStyle(MakeArea("UNKNWN"));
  if (!Expect(!fallback_area.visible,
              "Unknown area should not force a generic solid-fill fallback.")) {
    return 1;
  }

  const auto fallback_line = navscene::render::ResolveLinePaintStyle(MakeLine("UNKNWN"));
  if (!Expect(fallback_line.visible && fallback_line.width == 1,
              "Unknown line should fall back to the default line palette.")) {
    return 1;
  }

  const auto fallback_point = navscene::render::ResolvePointPaintStyle(MakePoint("UNKNWN"));
  if (!Expect(fallback_point.visible && fallback_point.radius == 2,
              "Unknown point should fall back to the default point palette.")) {
    return 1;
  }

  auto hidden_meta_settings = navscene::portrayal::MakeDisplaySettings(navscene::DisplayOptions{});
  hidden_meta_settings.display_category = navscene::DisplayCategory::kAll;
  const auto hidden_meta_scene =
      navscene::portrayal::BuildPortrayalScene(BuildFilterScene(), hidden_meta_settings);
  if (!Expect(hidden_meta_scene.areas.size() == 1,
              "Default portrayal settings should hide metadata areas.")) {
    return 1;
  }
  if (!Expect(hidden_meta_scene.points.size() == 2,
              "Default portrayal settings should keep soundings and buoy markers.")) {
    return 1;
  }

  auto visible_meta_settings = hidden_meta_settings;
  visible_meta_settings.show_meta = true;
  const auto visible_meta_scene =
      navscene::portrayal::BuildPortrayalScene(BuildFilterScene(), visible_meta_settings);
  if (!Expect(visible_meta_scene.areas.size() == 2,
              "Enabling metadata should keep metadata areas in portrayal output.")) {
    return 1;
  }

  auto reduced_settings = visible_meta_settings;
  reduced_settings.show_soundings = false;
  reduced_settings.show_lights = false;
  const auto reduced_scene =
      navscene::portrayal::BuildPortrayalScene(BuildFilterScene(), reduced_settings);
  if (!Expect(reduced_scene.points.empty(),
              "Disabling soundings and lights should filter both sample point portrayals.")) {
    return 1;
  }

  navscene::render::ChartScene restriction_scene;
  restriction_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 5,
      .object_class_acronym = "RESARE",
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto restriction_output =
      navscene::portrayal::BuildPortrayalScene(restriction_scene, visible_meta_settings);
  if (!Expect(restriction_output.areas.size() == 1,
              "Restriction areas should still be emitted into portrayal output.")) {
    return 1;
  }
  if (!Expect(!restriction_output.areas.front().fill.enabled &&
                  restriction_output.areas.front().stroke.enabled &&
                  restriction_output.areas.front().stroke.pattern ==
                      navscene::portrayal::StrokePatternKind::kDash &&
                  restriction_output.areas.front().stroke.color.r == 197 &&
                  restriction_output.areas.front().stroke.color.g == 69 &&
                  restriction_output.areas.front().stroke.color.b == 195,
              "Restriction areas should render as boundary-only magenta features.")) {
    return 1;
  }

  navscene::render::ChartScene depth_scene;
  depth_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 6,
      .object_class_acronym = "DEPARE",
      .attributes = {{"DRVAL1", "0.0"}, {"DRVAL2", "1.0"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto depth_output =
      navscene::portrayal::BuildPortrayalScene(depth_scene, visible_meta_settings);
  if (!Expect(depth_output.areas.size() == 1,
              "Depth-area sample should be emitted into portrayal output.")) {
    return 1;
  }
  if (!Expect(depth_output.areas.front().fill.color.r == 115 &&
                  depth_output.areas.front().fill.color.g == 182 &&
                  depth_output.areas.front().fill.color.b == 239,
              "Shallow depth areas should use the darker shallow-water palette.")) {
    return 1;
  }

  navscene::render::ChartScene named_point_scene;
  named_point_scene.points.push_back(navscene::render::PointPrimitive{
      .dataset_path = "sample",
      .feature_id = 7,
      .object_class_acronym = "OFSPLF",
      .attributes = {{"OBJNAM", "Test Platform"}},
      .position = {.lat = 0.5, .lon = 0.5},
  });
  const auto named_point_output =
      navscene::portrayal::BuildPortrayalScene(named_point_scene, visible_meta_settings);
  if (!Expect(named_point_output.points.size() == 1,
              "Named platform point should be emitted into portrayal output.")) {
    return 1;
  }
  if (!Expect(named_point_output.points.front().symbol.kind ==
                  navscene::portrayal::PointSymbolKind::kSquare,
              "Named platform point should use the dedicated platform symbol.")) {
    return 1;
  }
  if (!Expect(named_point_output.labels.size() == 1 &&
                  named_point_output.labels.front().text == "Prod Test Platform",
              "Named platform point should generate a prefixed label through portrayal.")) {
    return 1;
  }

  navscene::render::ChartScene route_scene;
  route_scene.polylines.push_back(navscene::render::PolylinePrimitive{
      .dataset_path = "sample",
      .feature_id = 10,
      .object_class_acronym = "RECTRC",
      .attributes = {{"ORIENT", "45"}},
      .vertices = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto route_output =
      navscene::portrayal::BuildPortrayalScene(route_scene, visible_meta_settings);
  if (!Expect(route_output.lines.size() == 1 &&
                  route_output.lines.front().stroke.pattern ==
                      navscene::portrayal::StrokePatternKind::kDash,
              "RECTRC route lines should keep the dashed portrayal stroke.")) {
    return 1;
  }
  if (!Expect(route_output.labels.size() == 1 &&
                  route_output.labels.front().text == "045 deg",
              "RECTRC should synthesize the chart-like orientation label.")) {
    return 1;
  }

  navscene::render::ChartScene named_area_scene;
  named_area_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 11,
      .object_class_acronym = "PRDARE",
      .attributes = {{"OBJNAM", "Harbor Plant"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto named_area_output =
      navscene::portrayal::BuildPortrayalScene(named_area_scene, visible_meta_settings);
  if (!Expect(named_area_output.areas.size() == 1 &&
                  named_area_output.areas.front().stroke.pattern ==
                      navscene::portrayal::StrokePatternKind::kSolid &&
                  named_area_output.areas.front().stroke.width_px == 4,
              "PRDARE should use the dedicated emphasized production-area boundary.")) {
    return 1;
  }
  if (!Expect(named_area_output.labels.size() == 1 &&
                  named_area_output.labels.front().text == "Prod Harbor Plant",
              "PRDARE should generate prefixed production-area labels.")) {
    return 1;
  }

  navscene::render::ChartScene harbor_admin_scene;
  harbor_admin_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 111,
      .object_class_acronym = "HRBARE",
      .attributes = {{"OBJNAM", "Inner Harbour"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto harbor_admin_output =
      navscene::portrayal::BuildPortrayalScene(harbor_admin_scene, visible_meta_settings);
  if (!Expect(harbor_admin_output.areas.size() == 1 &&
                  !harbor_admin_output.areas.front().fill.enabled &&
                  harbor_admin_output.areas.front().stroke.enabled &&
                  harbor_admin_output.areas.front().stroke.pattern ==
                      navscene::portrayal::StrokePatternKind::kDash &&
                  harbor_admin_output.areas.front().stroke.width_px == 2 &&
                  harbor_admin_output.areas.front().stroke.color.r == 125 &&
                  harbor_admin_output.areas.front().stroke.color.g == 137 &&
                  harbor_admin_output.areas.front().stroke.color.b == 140,
              "HRBARE should render as a dashed administrative boundary so harbour water stays visible.")) {
    return 1;
  }

  navscene::render::ChartScene railway_scene;
  railway_scene.polylines.push_back(navscene::render::PolylinePrimitive{
      .dataset_path = "sample",
      .feature_id = 12,
      .object_class_acronym = "RAILWY",
      .vertices = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto railway_output =
      navscene::portrayal::BuildPortrayalScene(railway_scene, visible_meta_settings);
  if (!Expect(railway_output.lines.size() == 1 &&
                  railway_output.lines.front().stroke.width_px == 2,
              "RAILWY should use the heavier railway stroke width.")) {
    return 1;
  }

  navscene::render::ChartScene slcons_scene;
  slcons_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 14,
      .object_class_acronym = "SLCONS",
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto slcons_output =
      navscene::portrayal::BuildPortrayalScene(slcons_scene, visible_meta_settings);
  if (!Expect(slcons_output.areas.size() == 1 &&
                  !slcons_output.areas.front().fill.enabled &&
                  slcons_output.areas.front().stroke.enabled &&
                  slcons_output.areas.front().stroke.color.r == 82 &&
                  slcons_output.areas.front().stroke.color.g == 90 &&
                  slcons_output.areas.front().stroke.color.b == 92 &&
                  slcons_output.areas.front().stroke.width_px == 2,
              "SLCONS areas should render as outline-only shoreline constructions with the coastline stroke palette instead of opaque fills.")) {
    return 1;
  }

  navscene::render::ChartScene low_accuracy_slcons_scene;
  low_accuracy_slcons_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 141,
      .object_class_acronym = "SLCONS",
      .attributes = {{"QUAPOS", "4"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto low_accuracy_slcons_output =
      navscene::portrayal::BuildPortrayalScene(low_accuracy_slcons_scene, visible_meta_settings);
  if (!Expect(low_accuracy_slcons_output.areas.size() == 1 &&
                  !low_accuracy_slcons_output.areas.front().fill.enabled &&
                  low_accuracy_slcons_output.areas.front().stroke.enabled &&
                  low_accuracy_slcons_output.areas.front().stroke.pattern ==
                      navscene::portrayal::StrokePatternKind::kDot &&
                  low_accuracy_slcons_output.areas.front().stroke.width_px == 2 &&
                  low_accuracy_slcons_output.areas.front().stroke.color.r == 82 &&
                  low_accuracy_slcons_output.areas.front().stroke.color.g == 90 &&
                  low_accuracy_slcons_output.areas.front().stroke.color.b == 92,
              "Low-accuracy SLCONS areas should route through the execution-layer shoreline quality treatment instead of the old procedural fallback.")) {
    return 1;
  }

  navscene::render::ChartScene roadway_scene;
  roadway_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 13,
      .object_class_acronym = "ROADWY",
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto roadway_output =
      navscene::portrayal::BuildPortrayalScene(roadway_scene, visible_meta_settings);
  if (!Expect(roadway_output.areas.size() == 1 &&
                  roadway_output.areas.front().fill.enabled &&
                  roadway_output.areas.front().fill.color.r == 201,
              "ROADWY areas should use the dedicated land-toned road-area fill.")) {
    return 1;
  }

  navscene::render::ChartScene river_scene;
  river_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 22,
      .object_class_acronym = "RIVERS",
      .attributes = {{"OBJNAM", "Test River"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto river_output =
      navscene::portrayal::BuildPortrayalScene(river_scene, visible_meta_settings);
  if (!Expect(river_output.areas.size() == 1 &&
                  river_output.areas.front().fill.enabled &&
                  river_output.areas.front().fill.color.r == 115 &&
                  river_output.areas.front().fill.color.g == 182 &&
                  river_output.areas.front().fill.color.b == 239 &&
                  river_output.areas.front().stroke.enabled &&
                  river_output.areas.front().stroke.color.r == 7,
              "RIVERS areas should render as blue inland water instead of falling through land.")) {
    return 1;
  }

  navscene::render::ChartScene canal_scene;
  canal_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 122,
      .object_class_acronym = "CANALS",
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto canal_output =
      navscene::portrayal::BuildPortrayalScene(canal_scene, visible_meta_settings);
  if (!Expect(canal_output.areas.size() == 1 &&
                  canal_output.areas.front().fill.enabled &&
                  canal_output.areas.front().fill.color.r == 115 &&
                  canal_output.areas.front().fill.color.g == 182 &&
                  canal_output.areas.front().fill.color.b == 239 &&
                  canal_output.areas.front().stroke.enabled &&
                  canal_output.areas.front().stroke.color.r == 7 &&
                  canal_output.areas.front().stroke.color.g == 7 &&
                  canal_output.areas.front().stroke.color.b == 7,
              "CANALS areas should use the same blue-and-black inland-water treatment as S-52 display-base water features.")) {
    return 1;
  }

  navscene::render::ChartScene overlap_scene;
  overlap_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 300,
      .object_class_acronym = "LNDARE",
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 2.0},
          {.lat = 2.0, .lon = 2.0},
      },
  });
  overlap_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 301,
      .object_class_acronym = "CANALS",
      .outer_ring = {
          {.lat = 0.5, .lon = 0.5},
          {.lat = 0.5, .lon = 1.5},
          {.lat = 1.5, .lon = 1.5},
      },
  });
  const auto overlap_output =
      navscene::portrayal::BuildPortrayalScene(overlap_scene, visible_meta_settings);
  if (!Expect(overlap_output.areas.size() == 2 &&
                  overlap_output.areas.front().geometry.object_class_acronym == "LNDARE" &&
                  overlap_output.areas.back().geometry.object_class_acronym == "CANALS",
              "Inland-water areas should sort above LNDARE so canals and dock water are not painted over by land.")) {
    return 1;
  }

  navscene::render::ChartScene tideway_scene;
  tideway_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 123,
      .object_class_acronym = "TIDEWY",
      .attributes = {{"OBJNAM", "Old Channel"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto tideway_output =
      navscene::portrayal::BuildPortrayalScene(tideway_scene, visible_meta_settings);
  if (!Expect(tideway_output.areas.empty() &&
                  tideway_output.labels.size() == 1 &&
                  tideway_output.labels.front().text == "Old Channel",
              "TIDEWY areas should be allowed to contribute labels even when the portrayal rule is label-only.")) {
    return 1;
  }

  navscene::render::ChartScene seabed_scene;
  seabed_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 223,
      .object_class_acronym = "SBDARE",
      .attributes = {{"NATSUR", "(2:4,17)"}, {"WATLEV", "3"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto seabed_output =
      navscene::portrayal::BuildPortrayalScene(seabed_scene, visible_meta_settings);
  if (!Expect(seabed_output.areas.size() == 1 &&
                  !seabed_output.areas.front().fill.enabled &&
                  seabed_output.areas.front().stroke.enabled &&
                  seabed_output.areas.front().stroke.pattern ==
                      navscene::portrayal::StrokePatternKind::kDash &&
                  seabed_output.labels.size() == 1 &&
                  seabed_output.labels.front().text == "S Sh",
              "SBDARE should emit the S-52 seabed abbreviation label and a dashed drying-area boundary.")) {
    return 1;
  }

  navscene::render::ChartScene named_sea_scene;
  named_sea_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 224,
      .object_class_acronym = "SEAARE",
      .attributes = {{"OBJNAM", "Pillars Shoal"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto named_sea_output =
      navscene::portrayal::BuildPortrayalScene(named_sea_scene, visible_meta_settings);
  if (!Expect(named_sea_output.areas.empty() &&
                  named_sea_output.labels.size() == 1 &&
                  named_sea_output.labels.front().text == "Pillars Shoal",
              "SEAARE should contribute named water-area labels even without an area fill.")) {
    return 1;
  }

  navscene::render::ChartScene cable_area_scene;
  cable_area_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 225,
      .object_class_acronym = "CBLARE",
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto cable_area_output =
      navscene::portrayal::BuildPortrayalScene(cable_area_scene, visible_meta_settings);
  if (!Expect(cable_area_output.areas.size() == 1 &&
                  cable_area_output.areas.front().overlays.size() == 1 &&
                  cable_area_output.areas.front().overlays.front().kind ==
                      navscene::portrayal::AreaOverlayKind::kCableBoundary &&
                  !cable_area_output.areas.front().fill.enabled &&
                  cable_area_output.areas.front().stroke.enabled &&
                  cable_area_output.areas.front().stroke.pattern ==
                      navscene::portrayal::StrokePatternKind::kDash &&
                  cable_area_output.areas.front().stroke.color.r == 197 &&
                  cable_area_output.areas.front().stroke.color.g == 69 &&
                  cable_area_output.areas.front().stroke.color.b == 195,
              "CBLARE should render as a magenta dashed cable-area boundary.")) {
    return 1;
  }

  navscene::render::ChartScene bridge_area_scene;
  bridge_area_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 226,
      .object_class_acronym = "BRIDGE",
      .attributes = {{"VERCLR", "9.0"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto bridge_area_output =
      navscene::portrayal::BuildPortrayalScene(bridge_area_scene, visible_meta_settings);
  if (!Expect(bridge_area_output.areas.size() == 1 &&
                  bridge_area_output.areas.front().stroke.enabled &&
                  bridge_area_output.areas.front().stroke.width_px == 4 &&
                  bridge_area_output.labels.size() == 1 &&
                  bridge_area_output.labels.front().text == "clr 9.0",
              "BRIDGE areas should render an emphasized outline and synthesize a clearance label.")) {
    return 1;
  }

  navscene::render::ChartScene named_buoy_scene;
  named_buoy_scene.points.push_back(navscene::render::PointPrimitive{
      .dataset_path = "sample",
      .feature_id = 8,
      .object_class_acronym = "BOYLAT",
      .attributes = {{"OBJNAM", "Test Buoy"}},
      .position = {.lat = 0.5, .lon = 0.5},
  });
  const auto named_buoy_output =
      navscene::portrayal::BuildPortrayalScene(named_buoy_scene, visible_meta_settings);
  if (!Expect(named_buoy_output.labels.size() == 1 &&
                  named_buoy_output.labels.front().text == "by Test Buoy",
              "Named buoy should generate a navigation-style prefixed label.")) {
    return 1;
  }

  navscene::render::ChartScene light_scene;
  light_scene.points.push_back(navscene::render::PointPrimitive{
      .dataset_path = "sample",
      .feature_id = 9,
      .object_class_acronym = "LIGHTS",
      .attributes = {{"LITCHR", "2"},
                     {"SIGGRP", "(1)"},
                     {"COLOUR", "6"},
                     {"SIGPER", "5"},
                     {"VALNMR", "22"}},
      .position = {.lat = 0.5, .lon = 0.5},
  });
  const auto light_output =
      navscene::portrayal::BuildPortrayalScene(light_scene, visible_meta_settings);
  if (!Expect(light_output.points.size() == 1 &&
                  light_output.points.front().symbol.kind ==
                      navscene::portrayal::PointSymbolKind::kDiamond &&
                  light_output.points.front().symbol.fill.r == 244 &&
                  light_output.points.front().symbol.fill.g == 218 &&
                  light_output.points.front().symbol.fill.b == 72 &&
                  light_output.labels.size() == 1 &&
                  light_output.labels.front().text == "Fl(1)Y 5s 22M",
              "LIGHTS should derive a light-colored point symbol and synthesize a more chart-like light signature label.")) {
    return 1;
  }

  navscene::render::ChartScene leading_light_scene;
  leading_light_scene.points.push_back(navscene::render::PointPrimitive{
      .dataset_path = "sample",
      .feature_id = 211,
      .object_class_acronym = "LIGHTS",
      .attributes = {{"COLOUR", "3"}, {"CATLIT", "(1:1)"}},
      .position = {.lat = 0.5, .lon = 0.5},
  });
  const auto leading_light_output =
      navscene::portrayal::BuildPortrayalScene(leading_light_scene, visible_meta_settings);
  if (!Expect(leading_light_output.points.size() == 1 &&
                  leading_light_output.points.front().symbol.size_px == 8 &&
                  leading_light_output.points.front().symbol.fill.r == 241 &&
                  leading_light_output.points.front().symbol.fill.g == 84 &&
                  leading_light_output.points.front().symbol.fill.b == 105,
              "Leading LIGHTS points should be slightly emphasized and resolve their fill from the light color.")) {
    return 1;
  }

  navscene::render::ChartScene topmark_scene;
  topmark_scene.points.push_back(navscene::render::PointPrimitive{
      .dataset_path = "sample",
      .feature_id = 15,
      .object_class_acronym = "TOPMAR",
      .attributes = {{"TOPSHP", "12"}, {"COLOUR", "11,6"}},
      .position = {.lat = 0.5, .lon = 0.5},
  });
  const auto topmark_output =
      navscene::portrayal::BuildPortrayalScene(topmark_scene, visible_meta_settings);
  if (!Expect(topmark_output.points.size() == 1 &&
                  topmark_output.points.front().symbol.kind ==
                      navscene::portrayal::PointSymbolKind::kDiamond &&
                  topmark_output.points.front().symbol.fill.r == 244 &&
                  topmark_output.points.front().symbol.fill.g == 218 &&
                  topmark_output.points.front().symbol.fill.b == 72 &&
                  topmark_output.points.front().symbol.stroke.r == 7 &&
                  topmark_output.points.front().symbol.stroke.g == 7 &&
                  topmark_output.points.front().symbol.stroke.b == 7,
              "TOPMAR should derive shape and colors from TOPSHP and COLOUR attributes.")) {
    return 1;
  }

  navscene::render::ChartScene obstruction_area_scene;
  obstruction_area_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 16,
      .object_class_acronym = "OBSTRN",
      .attributes = {{"CATOBS", "6"}, {"WATLEV", "3"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto obstruction_area_output =
      navscene::portrayal::BuildPortrayalScene(obstruction_area_scene, visible_meta_settings);
  if (!Expect(obstruction_area_output.areas.size() == 1 &&
                  obstruction_area_output.areas.front().fill.enabled &&
                  obstruction_area_output.areas.front().fill.color.r == 131 &&
                  obstruction_area_output.areas.front().fill.color.g == 178 &&
                  obstruction_area_output.areas.front().fill.color.b == 149 &&
                  obstruction_area_output.areas.front().stroke.pattern ==
                      navscene::portrayal::StrokePatternKind::kDot &&
                  obstruction_area_output.areas.front().stroke.width_px == 2 &&
                  obstruction_area_output.areas.front().stroke.color.r == 7,
              "OBSTRN CATOBS=6 areas should use a danger fill with dotted black outline.")) {
    return 1;
  }

  navscene::render::ChartScene protected_area_scene;
  protected_area_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 17,
      .object_class_acronym = "RESARE",
      .attributes = {{"CATREA", "(1:10)"}, {"RESTRN", "(1:14)"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto protected_area_output =
      navscene::portrayal::BuildPortrayalScene(protected_area_scene, visible_meta_settings);
  if (!Expect(protected_area_output.areas.size() == 1 &&
                  protected_area_output.areas.front().fill.enabled &&
                  protected_area_output.areas.front().fill.color.r == 211 &&
                  protected_area_output.areas.front().fill.color.g == 166 &&
                  protected_area_output.areas.front().fill.color.b == 233 &&
                  protected_area_output.areas.front().stroke.pattern ==
                      navscene::portrayal::StrokePatternKind::kDash,
              "Special-use RESARE areas should gain an emphasized light-magenta fill.")) {
    return 1;
  }

  navscene::render::ChartScene traffic_zone_scene;
  traffic_zone_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 171,
      .object_class_acronym = "TSEZNE",
      .attributes = {{"CATTSS", "1"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto traffic_zone_output =
      navscene::portrayal::BuildPortrayalScene(traffic_zone_scene, visible_meta_settings);
  const bool valid_traffic_zone = traffic_zone_output.areas.size() == 1 &&
                                  traffic_zone_output.areas.front().fill.enabled &&
                                  traffic_zone_output.areas.front().fill.color.r >= 210 &&
                                  traffic_zone_output.areas.front().fill.color.r <= 213 &&
                                  traffic_zone_output.areas.front().fill.color.g >= 221 &&
                                  traffic_zone_output.areas.front().fill.color.g <= 223 &&
                                  traffic_zone_output.areas.front().fill.color.b >= 235 &&
                                  traffic_zone_output.areas.front().fill.color.b <= 237 &&
                                  !traffic_zone_output.areas.front().stroke.enabled;
  if (!valid_traffic_zone) {
    if (!traffic_zone_output.areas.empty()) {
      const auto& fill = traffic_zone_output.areas.front().fill.color;
      std::cerr << "[navscene-portrayal-mvp] TSEZNE actual rgb="
                << static_cast<int>(fill.r) << ','
                << static_cast<int>(fill.g) << ','
                << static_cast<int>(fill.b)
                << " stroke_enabled=" << traffic_zone_output.areas.front().stroke.enabled
                << '\n';
    }
  }
  if (!Expect(valid_traffic_zone,
              "TSEZNE areas should render with a pale traffic-separation overlay fill.")) {
    return 1;
  }

  navscene::render::ChartScene rock_hazard_scene;
  rock_hazard_scene.points.push_back(navscene::render::PointPrimitive{
      .dataset_path = "sample",
      .feature_id = 18,
      .object_class_acronym = "UWTROC",
      .attributes = {{"WATLEV", "4"}, {"VALSOU", "-0.6"}},
      .position = {.lat = 0.5, .lon = 0.5},
  });
  const auto rock_hazard_output =
      navscene::portrayal::BuildPortrayalScene(rock_hazard_scene, visible_meta_settings);
  if (!Expect(rock_hazard_output.points.size() == 1 &&
                  rock_hazard_output.points.front().symbol.fill.r == 7 &&
                  rock_hazard_output.points.front().symbol.fill.g == 7 &&
                  rock_hazard_output.points.front().symbol.fill.b == 7 &&
                  rock_hazard_output.labels.size() == 1 &&
                  rock_hazard_output.labels.front().text == "-0.6",
              "Dangerous UWTROC points should emphasize the hazard and label the depth.")) {
    return 1;
  }

  navscene::render::ChartScene anchorage_area_scene;
  anchorage_area_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 19,
      .object_class_acronym = "ACHARE",
      .attributes = {{"CATACH", "(1:1)"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto anchorage_area_output =
      navscene::portrayal::BuildPortrayalScene(anchorage_area_scene, visible_meta_settings);
  if (!Expect(anchorage_area_output.areas.size() == 1 &&
                  !anchorage_area_output.areas.front().fill.enabled &&
                  anchorage_area_output.areas.front().stroke.pattern ==
                      navscene::portrayal::StrokePatternKind::kDash &&
                  anchorage_area_output.areas.front().stroke.width_px == 2 &&
                  anchorage_area_output.areas.front().stroke.color.r == 211,
              "ACHARE areas should use the lighter dashed anchorage boundary treatment.")) {
    return 1;
  }

  navscene::render::ChartScene platform_area_scene;
  platform_area_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 124,
      .object_class_acronym = "OFSPLF",
      .attributes = {{"OBJNAM", "Well A"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto platform_area_output =
      navscene::portrayal::BuildPortrayalScene(platform_area_scene, visible_meta_settings);
  if (!Expect(platform_area_output.areas.size() == 1 &&
                  platform_area_output.areas.front().stroke.width_px == 4 &&
                  platform_area_output.areas.front().stroke.color.r == 82 &&
                  platform_area_output.labels.size() == 1 &&
                  platform_area_output.labels.front().text == "Prod Well A",
              "OFSPLF areas should use the emphasized production-platform boundary and prefixed label.")) {
    return 1;
  }

  navscene::render::ChartScene wreck_scene;
  wreck_scene.points.push_back(navscene::render::PointPrimitive{
      .dataset_path = "sample",
      .feature_id = 20,
      .object_class_acronym = "WRECKS",
      .attributes = {{"CATWRK", "2"}, {"WATLEV", "3"}, {"VALSOU", "1.8"}},
      .position = {.lat = 0.5, .lon = 0.5},
  });
  const auto wreck_output =
      navscene::portrayal::BuildPortrayalScene(wreck_scene, visible_meta_settings);
  if (!Expect(wreck_output.points.size() == 1 &&
                  wreck_output.points.front().symbol.fill.r == 115 &&
                  wreck_output.points.front().symbol.fill.g == 182 &&
                  wreck_output.points.front().symbol.fill.b == 239 &&
                  wreck_output.labels.size() == 1 &&
                  wreck_output.labels.front().text == "1.8",
              "Dangerous WRECKS points should use the shallow-hazard palette and depth label.")) {
    return 1;
  }

  navscene::render::ChartScene estimated_wreck_scene;
  estimated_wreck_scene.points.push_back(navscene::render::PointPrimitive{
      .dataset_path = "sample",
      .feature_id = 208,
      .object_class_acronym = "WRECKS",
      .attributes = {{"CATWRK", "2"}},
      .position = {.lat = 0.5, .lon = 0.5},
  });
  const auto estimated_wreck_output =
      navscene::portrayal::BuildPortrayalScene(estimated_wreck_scene, visible_meta_settings);
  if (!Expect(estimated_wreck_output.points.size() == 1 &&
                  estimated_wreck_output.points.front().symbol.kind ==
                      navscene::portrayal::PointSymbolKind::kDiamond &&
                  estimated_wreck_output.points.front().symbol.fill.r == 115 &&
                  estimated_wreck_output.points.front().symbol.fill.g == 182 &&
                  estimated_wreck_output.points.front().symbol.fill.b == 239,
              "WRECKS CATWRK=2 points without VALSOU should still resolve as dangerous hazards through estimated depth.")) {
    return 1;
  }

  navscene::render::ChartScene contour_scene;
  contour_scene.polylines.push_back(navscene::render::PolylinePrimitive{
      .dataset_path = "sample",
      .feature_id = 209,
      .object_class_acronym = "DEPCNT",
      .attributes = {{"VALDCO", "30"}, {"QUAPOS", "4"}},
      .vertices = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto contour_output =
      navscene::portrayal::BuildPortrayalScene(contour_scene, visible_meta_settings);
  if (!Expect(contour_output.lines.size() == 1 &&
                  contour_output.lines.front().stroke.pattern ==
                      navscene::portrayal::StrokePatternKind::kDash &&
                  contour_output.lines.front().stroke.width_px == 2 &&
                  contour_output.lines.front().stroke.color.r == 82 &&
                  contour_output.lines.front().stroke.color.g == 90 &&
                  contour_output.lines.front().stroke.color.b == 92,
              "Safety DEPCNT lines with low positional accuracy should render as emphasized dashed safety contours.")) {
    return 1;
  }

  auto quality_settings = visible_meta_settings;
  quality_settings.show_quality_of_data = true;
  navscene::render::ChartScene quality_scene;
  quality_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 21,
      .object_class_acronym = "M_QUAL",
      .attributes = {{"CATZOC", "4"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto quality_output =
      navscene::portrayal::BuildPortrayalScene(quality_scene, quality_settings);
  if (!Expect(quality_output.areas.size() == 1 &&
                  !quality_output.areas.front().fill.enabled &&
                  quality_output.areas.front().overlays.size() == 1 &&
                  quality_output.areas.front().overlays.front().kind ==
                      navscene::portrayal::AreaOverlayKind::kDataQualityC &&
                  quality_output.areas.front().stroke.pattern ==
                      navscene::portrayal::StrokePatternKind::kDash &&
                  quality_output.areas.front().stroke.width_px == 2,
              "M_QUAL areas should render as a pattern-only quality overlay with a dashed metadata boundary.")) {
    return 1;
  }

  auto dusk_settings = visible_meta_settings;
  dusk_settings.color_scheme = navscene::ColorScheme::kDusk;
  const auto dusk_depth_output =
      navscene::portrayal::BuildPortrayalScene(depth_scene, dusk_settings);
  if (!Expect(dusk_depth_output.background_color.r == 7 &&
                  dusk_depth_output.background_color.g == 7 &&
                  dusk_depth_output.background_color.b == 7,
              "Dusk portrayal should resolve the background through the dusk palette.")) {
    return 1;
  }
  if (!Expect(dusk_depth_output.areas.size() == 1 &&
                  dusk_depth_output.areas.front().fill.color.r == 22 &&
                  dusk_depth_output.areas.front().fill.color.g == 35 &&
                  dusk_depth_output.areas.front().fill.color.b == 47,
              "Dusk portrayal should resolve depth fills through palette indirection.")) {
    return 1;
  }

  navscene::render::ChartScene drying_depth_scene;
  drying_depth_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 200,
      .object_class_acronym = "DEPARE",
      .attributes = {{"DRVAL1", "-5"}, {"DRVAL2", "0"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto drying_depth_output =
      navscene::portrayal::BuildPortrayalScene(drying_depth_scene, visible_meta_settings);
  if (!Expect(drying_depth_output.areas.size() == 1 &&
                  drying_depth_output.areas.front().fill.color.r == 131 &&
                  drying_depth_output.areas.front().fill.color.g == 178 &&
                  drying_depth_output.areas.front().fill.color.b == 149,
              "Drying DEPARE ranges should use the intertidal S-52 palette.")) {
    return 1;
  }

  navscene::render::ChartScene unknown_depth_area_scene;
  unknown_depth_area_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 210,
      .object_class_acronym = "DEPARE",
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto unknown_depth_area_output =
      navscene::portrayal::BuildPortrayalScene(unknown_depth_area_scene, visible_meta_settings);
  if (!Expect(unknown_depth_area_output.areas.size() == 1 &&
                  unknown_depth_area_output.areas.front().fill.enabled &&
                  unknown_depth_area_output.areas.front().fill.color.r == 163 &&
                  unknown_depth_area_output.areas.front().fill.color.g == 180 &&
                  unknown_depth_area_output.areas.front().fill.color.b == 183 &&
                  unknown_depth_area_output.areas.front().stroke.enabled &&
                  unknown_depth_area_output.areas.front().stroke.width_px == 2 &&
                  unknown_depth_area_output.areas.front().overlays.size() == 1 &&
                  unknown_depth_area_output.areas.front().overlays.front().kind ==
                      navscene::portrayal::AreaOverlayKind::kSurveyReliability,
              "DEPARE areas without depth values should render as no-data water with the survey-reliability overlay.")) {
    return 1;
  }

  navscene::render::ChartScene ledge_scene;
  ledge_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 202,
      .object_class_acronym = "SBDARE",
      .attributes = {{"WATLEV", "4"}, {"NATSUR", "(1:9)"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto ledge_output =
      navscene::portrayal::BuildPortrayalScene(ledge_scene, visible_meta_settings);
  if (!Expect(ledge_output.areas.size() == 1 &&
                  ledge_output.areas.front().overlays.size() == 1 &&
                  ledge_output.areas.front().overlays.front().kind ==
                      navscene::portrayal::AreaOverlayKind::kRockLedge &&
                  ledge_output.areas.front().stroke.enabled &&
                  ledge_output.areas.front().stroke.pattern ==
                      navscene::portrayal::StrokePatternKind::kDash,
              "Drying SBDARE rock ledges should emit the dedicated rock-ledge overlay plus dashed boundary.")) {
    return 1;
  }

  navscene::render::ChartScene airport_area_scene;
  airport_area_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 203,
      .object_class_acronym = "AIRARE",
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto airport_area_output =
      navscene::portrayal::BuildPortrayalScene(airport_area_scene, visible_meta_settings);
  if (!Expect(airport_area_output.areas.size() == 1 &&
                  airport_area_output.areas.front().overlays.size() == 1 &&
                  airport_area_output.areas.front().overlays.front().kind ==
                      navscene::portrayal::AreaOverlayKind::kAirport &&
                  !airport_area_output.areas.front().fill.enabled,
              "AIRARE areas should emit the dedicated airport-area overlay symbol pattern without forcing land fill when CONVIS is absent.")) {
    return 1;
  }

  navscene::render::ChartScene conspicuous_airport_scene;
  conspicuous_airport_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 204,
      .object_class_acronym = "AIRARE",
      .attributes = {{"CONVIS", "1"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto conspicuous_airport_output =
      navscene::portrayal::BuildPortrayalScene(conspicuous_airport_scene, visible_meta_settings);
  if (!Expect(conspicuous_airport_output.areas.size() == 1 &&
                  conspicuous_airport_output.areas.front().fill.enabled &&
                  conspicuous_airport_output.areas.front().stroke.enabled &&
                  conspicuous_airport_output.areas.front().stroke.color.r == 7,
              "AIRARE CONVIS=1 areas should retain the conspicuous land fill and black outline.")) {
    return 1;
  }

  navscene::render::ChartScene no_data_area_scene;
  no_data_area_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 205,
      .object_class_acronym = "UNSARE",
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto no_data_area_output =
      navscene::portrayal::BuildPortrayalScene(no_data_area_scene, visible_meta_settings);
  if (!Expect(no_data_area_output.areas.size() == 1 &&
                  no_data_area_output.areas.front().fill.enabled &&
                  no_data_area_output.areas.front().fill.color.r == 163 &&
                  no_data_area_output.areas.front().fill.color.g == 180 &&
                  no_data_area_output.areas.front().fill.color.b == 183 &&
                  no_data_area_output.areas.front().overlays.size() == 1 &&
                  no_data_area_output.areas.front().overlays.front().kind ==
                      navscene::portrayal::AreaOverlayKind::kNoDataArea,
              "UNSARE areas should render with the no-data fill and pattern overlay.")) {
    return 1;
  }

  navscene::render::ChartScene dredged_area_scene;
  dredged_area_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 207,
      .object_class_acronym = "DRGARE",
      .attributes = {{"DRVAL1", "8.6"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto dredged_area_output =
      navscene::portrayal::BuildPortrayalScene(dredged_area_scene, visible_meta_settings);
  if (!Expect(dredged_area_output.areas.size() == 1 &&
                  dredged_area_output.areas.front().fill.enabled &&
                  dredged_area_output.areas.front().stroke.enabled &&
                  dredged_area_output.areas.front().stroke.pattern ==
                      navscene::portrayal::StrokePatternKind::kDash &&
                  dredged_area_output.areas.front().stroke.width_px == 1 &&
                  dredged_area_output.areas.front().overlays.size() == 1 &&
                  dredged_area_output.areas.front().overlays.front().kind ==
                      navscene::portrayal::AreaOverlayKind::kDredgedArea,
              "DRGARE areas should keep depth-based fill while adding the dredged-area dash and pattern overlay.")) {
    return 1;
  }

  navscene::render::ChartScene vegetation_scene;
  vegetation_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 206,
      .object_class_acronym = "VEGATN",
      .attributes = {{"CATVEG", "(1:17)"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto vegetation_output =
      navscene::portrayal::BuildPortrayalScene(vegetation_scene, visible_meta_settings);
  if (!Expect(vegetation_output.areas.size() == 1 &&
                  !vegetation_output.areas.front().fill.enabled &&
                  vegetation_output.areas.front().stroke.enabled &&
                  vegetation_output.areas.front().stroke.pattern ==
                      navscene::portrayal::StrokePatternKind::kDash &&
                  vegetation_output.areas.front().overlays.size() == 1 &&
                  vegetation_output.areas.front().overlays.front().kind ==
                      navscene::portrayal::AreaOverlayKind::kVegetationWooded,
              "VEGATN wooded areas should render as patterned land overlays with a dashed outline.")) {
    return 1;
  }

  auto safety_depth_settings = visible_meta_settings;
  safety_depth_settings.safety_contour_m = 8.0;
  safety_depth_settings.safety_depth_m = 8.0;
  safety_depth_settings.deep_contour_m = 8.0;
  navscene::render::ChartScene medium_depth_scene;
  medium_depth_scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "sample",
      .feature_id = 201,
      .object_class_acronym = "DEPARE",
      .attributes = {{"DRVAL1", "5"}, {"DRVAL2", "8.6"}},
      .outer_ring = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 0.0, .lon = 1.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto medium_depth_output =
      navscene::portrayal::BuildPortrayalScene(medium_depth_scene, safety_depth_settings);
  if (!Expect(medium_depth_output.areas.size() == 1 &&
                  medium_depth_output.areas.front().fill.color.r == 152 &&
                  medium_depth_output.areas.front().fill.color.g == 197 &&
                  medium_depth_output.areas.front().fill.color.b == 242,
              "DEPARE ranges spanning the 8m safety contour should stay in the medium safe-water band.")) {
    return 1;
  }

  navscene::render::ChartScene low_accuracy_line_scene;
  low_accuracy_line_scene.polylines.push_back(navscene::render::PolylinePrimitive{
      .dataset_path = "sample",
      .feature_id = 211,
      .object_class_acronym = "COALNE",
      .attributes = {{"QUAPOS", "4"}},
      .vertices = {
          {.lat = 0.0, .lon = 0.0},
          {.lat = 1.0, .lon = 1.0},
      },
  });
  const auto low_accuracy_line_output =
      navscene::portrayal::BuildPortrayalScene(low_accuracy_line_scene, visible_meta_settings);
  if (!Expect(low_accuracy_line_output.lines.size() == 1 &&
                  low_accuracy_line_output.lines.front().stroke.pattern ==
                      navscene::portrayal::StrokePatternKind::kDot &&
                  low_accuracy_line_output.lines.front().stroke.width_px == 2 &&
                  low_accuracy_line_output.lines.front().stroke.color.r == 82 &&
                  low_accuracy_line_output.lines.front().stroke.color.g == 90 &&
                  low_accuracy_line_output.lines.front().stroke.color.b == 92,
              "COALNE lines with QUAPOS low-accuracy metadata should switch to the QUALIN-style dotted coastline treatment.")) {
    return 1;
  }

  navscene::render::ChartScene low_accuracy_point_scene;
  low_accuracy_point_scene.points.push_back(navscene::render::PointPrimitive{
      .dataset_path = "sample",
      .feature_id = 212,
      .object_class_acronym = "PILPNT",
      .attributes = {{"QUAPOS", "4"}},
      .position = {.lat = 0.5, .lon = 0.5},
  });
  const auto low_accuracy_point_output =
      navscene::portrayal::BuildPortrayalScene(low_accuracy_point_scene, visible_meta_settings);
  if (!Expect(low_accuracy_point_output.points.size() == 2 &&
                  low_accuracy_point_output.points.front().symbol.kind ==
                      navscene::portrayal::PointSymbolKind::kCircle &&
                  low_accuracy_point_output.points.back().symbol.kind ==
                      navscene::portrayal::PointSymbolKind::kTriangle &&
                  low_accuracy_point_output.points.back().symbol.stroke.r == 7 &&
                  low_accuracy_point_output.points.back().symbol.stroke.g == 7 &&
                  low_accuracy_point_output.points.back().symbol.stroke.b == 7,
              "Points with low QUAPOS accuracy should emit an additional clean-room positional-quality marker on top of the base symbol.")) {
    return 1;
  }

  return 0;
}
