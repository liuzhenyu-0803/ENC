#pragma once

#include "navscene/navscene.h"
#include "render/scene.h"

#include <cstdint>
#include <string>
#include <vector>

namespace navscene::portrayal {

struct Rgb8 {
  uint8_t r = 255;
  uint8_t g = 255;
  uint8_t b = 255;
};

struct FillStyle {
  Rgb8 color;
  std::string palette_color_id;
  bool enabled = true;
};

enum class AreaOverlayKind {
  kNone = 0,
  kAirport,
  kCableBoundary,
  kDataQualityA1,
  kDataQualityA2,
  kDataQualityB,
  kDataQualityC,
  kDataQualityD,
  kDataQualityUnknown,
  kNoData,
  kNoDataArea,
  kRockLedge,
  kDredgedArea,
  kSurveyReliability,
  kVegetationMangrove,
  kVegetationWooded,
};

enum class AreaOverlayPlacement {
  kTileInArea = 0,
  kAlongBoundary,
};

struct AreaOverlayStyle {
  AreaOverlayKind kind = AreaOverlayKind::kNone;
  AreaOverlayPlacement placement = AreaOverlayPlacement::kTileInArea;
  Rgb8 color;
  std::string palette_color_id;
  int spacing_px = 64;
  int line_width_px = 1;
  bool enabled = false;
};

enum class StrokePatternKind {
  kSolid = 0,
  kDash,
  kDot,
};

struct StrokeStyle {
  Rgb8 color;
  std::string palette_color_id;
  int width_px = 1;
  StrokePatternKind pattern = StrokePatternKind::kSolid;
  bool enabled = true;
};

enum class PointSymbolKind {
  kCircle = 0,
  kTriangle,
  kSquare,
  kDiamond,
};

struct PointSymbolStyle {
  PointSymbolKind kind = PointSymbolKind::kCircle;
  Rgb8 fill;
  std::string fill_palette_color_id;
  Rgb8 stroke;
  std::string stroke_palette_color_id;
  int size_px = 5;
  bool enabled = true;
};

enum class FontRole {
  kBody = 0,
  kImportant,
  kSounding,
};

struct TextStyle {
  FontRole role = FontRole::kBody;
  Rgb8 color;
  std::string palette_color_id;
  int size_px = 12;
  bool halo = false;
};

struct DisplaySettings {
  ColorScheme color_scheme = ColorScheme::kDay;
  DisplayCategory display_category = DisplayCategory::kStandard;
  bool show_text = true;
  bool show_soundings = true;
  bool show_lights = true;
  bool show_meta = false;
  bool show_quality_of_data = false;
  bool simplified_points = false;
  bool symbolized_boundaries = true;
  double safety_contour_m = 30.0;
  double safety_depth_m = 30.0;
  double shallow_contour_m = 2.0;
  double deep_contour_m = 30.0;
  double estimated_display_scale = 0.0;
};

inline DisplaySettings MakeDisplaySettings(const DisplayOptions& options) {
  return DisplaySettings{
      .color_scheme = options.color_scheme,
      .display_category = options.display_category,
      .show_text = options.show_text,
      .show_soundings = options.show_soundings,
      .show_lights = options.show_lights,
      .show_meta = options.show_meta,
      .show_quality_of_data = options.show_quality_of_data,
      .simplified_points = options.simplified_points,
      .symbolized_boundaries = options.symbolized_boundaries,
      .safety_contour_m = options.safety_contour_m,
      .safety_depth_m = options.safety_depth_m,
      .shallow_contour_m = options.shallow_contour_m,
      .deep_contour_m = options.deep_contour_m,
      .estimated_display_scale = 0.0,
  };
}

struct AreaCommand {
  int priority = 0;
  render::PolygonPrimitive geometry;
  FillStyle fill;
  StrokeStyle stroke;
  std::vector<AreaOverlayStyle> overlays;
  bool visible = true;
};

struct LineCommand {
  int priority = 0;
  render::PolylinePrimitive geometry;
  StrokeStyle stroke;
  bool visible = true;
};

struct PointCommand {
  int priority = 0;
  render::PointPrimitive geometry;
  PointSymbolStyle symbol;
  bool visible = true;
};

struct LabelCandidate {
  int priority = 0;
  std::string dataset_path;
  int64_t feature_id = -1;
  std::string object_class_acronym;
  std::string text;
  GeoPoint anchor;
  TextStyle style;
  bool important = false;
};

struct PortrayalSceneStats {
  uint64_t area_command_count = 0;
  uint64_t area_overlay_count = 0;
  uint64_t line_command_count = 0;
  uint64_t point_command_count = 0;
  uint64_t label_candidate_count = 0;
};

struct PortrayalScene {
  DisplaySettings display_settings;
  Rgb8 background_color{201, 235, 252};
  std::vector<AreaCommand> areas;
  std::vector<LineCommand> lines;
  std::vector<PointCommand> points;
  std::vector<LabelCandidate> labels;
  PortrayalSceneStats stats;
};

}  // namespace navscene::portrayal
