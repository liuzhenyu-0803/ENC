#include "portrayal/engine_internal.h"

#include <array>
#include <charconv>
#include <cmath>

namespace navscene::portrayal::detail {

Rgb8 MixColors(const Rgb8& foreground, const Rgb8& background, double foreground_weight) {
  const double clamped = std::clamp(foreground_weight, 0.0, 1.0);
  const double background_weight = 1.0 - clamped;
  return Rgb8{
      .r = static_cast<uint8_t>(
          std::lround(static_cast<double>(foreground.r) * clamped +
                      static_cast<double>(background.r) * background_weight)),
      .g = static_cast<uint8_t>(
          std::lround(static_cast<double>(foreground.g) * clamped +
                      static_cast<double>(background.g) * background_weight)),
      .b = static_cast<uint8_t>(
          std::lround(static_cast<double>(foreground.b) * clamped +
                      static_cast<double>(background.b) * background_weight)),
  };
}

Rgb8 ResolveBackgroundColor(const DisplaySettings& settings,
                            const S57PortrayalCatalog& catalog) {
  if (const auto* color = catalog.FindColor("DEPDW", settings.color_scheme)) {
    return *color;
  }
  if (const auto* sea_area = catalog.FindAreaStyle("sea_area")) {
    return ResolvePaletteColor(catalog,
                               settings,
                               sea_area->fill.palette_color_id,
                               sea_area->fill.color);
  }
  return Rgb8{201, 235, 252};
}

Rgb8 ResolvePaletteColor(const S57PortrayalCatalog& catalog,
                         const DisplaySettings& settings,
                         std::string_view palette_color_id,
                         const Rgb8& fallback) {
  if (!palette_color_id.empty()) {
    if (const auto* resolved = catalog.FindColor(palette_color_id, settings.color_scheme)) {
      return *resolved;
    }
  }
  return fallback;
}

FillStyle ResolveFillStyle(const FillStyle& style,
                           const DisplaySettings& settings,
                           const S57PortrayalCatalog& catalog) {
  FillStyle resolved = style;
  resolved.color =
      ResolvePaletteColor(catalog, settings, style.palette_color_id, style.color);
  return resolved;
}

AreaOverlayStyle ResolveAreaOverlayStyle(const AreaOverlayStyle& style,
                                         const DisplaySettings& settings,
                                         const S57PortrayalCatalog& catalog) {
  AreaOverlayStyle resolved = style;
  resolved.color =
      ResolvePaletteColor(catalog, settings, style.palette_color_id, style.color);
  return resolved;
}

StrokeStyle ResolveStrokeStyle(const StrokeStyle& style,
                               const DisplaySettings& settings,
                               const S57PortrayalCatalog& catalog) {
  StrokeStyle resolved = style;
  resolved.color =
      ResolvePaletteColor(catalog, settings, style.palette_color_id, style.color);
  return resolved;
}

PointSymbolStyle ResolvePointSymbolStyle(const PointSymbolStyle& style,
                                         const DisplaySettings& settings,
                                         const S57PortrayalCatalog& catalog) {
  PointSymbolStyle resolved = style;
  resolved.fill = ResolvePaletteColor(
      catalog, settings, style.fill_palette_color_id, style.fill);
  resolved.stroke = ResolvePaletteColor(
      catalog, settings, style.stroke_palette_color_id, style.stroke);
  return resolved;
}

TextStyle ResolveTextStyle(const TextStyle& style,
                           const DisplaySettings& settings,
                           const S57PortrayalCatalog& catalog) {
  TextStyle resolved = style;
  resolved.color =
      ResolvePaletteColor(catalog, settings, style.palette_color_id, style.color);
  return resolved;
}

void SortLabels(std::vector<LabelCandidate>* labels) {
  if (labels == nullptr) {
    return;
  }

  std::sort(labels->begin(), labels->end(), [](const LabelCandidate& lhs, const LabelCandidate& rhs) {
    if (lhs.priority != rhs.priority) {
      return lhs.priority < rhs.priority;
    }
    if (lhs.object_class_acronym != rhs.object_class_acronym) {
      return lhs.object_class_acronym < rhs.object_class_acronym;
    }
    if (lhs.dataset_path != rhs.dataset_path) {
      return lhs.dataset_path < rhs.dataset_path;
    }
    return lhs.feature_id < rhs.feature_id;
  });
}

void PopulateSceneStats(PortrayalScene* scene) {
  if (scene == nullptr) {
    return;
  }

  scene->stats.area_command_count = static_cast<uint64_t>(scene->areas.size());
  scene->stats.area_overlay_count = 0;
  for (const auto& area : scene->areas) {
    scene->stats.area_overlay_count += static_cast<uint64_t>(
        std::count_if(area.overlays.begin(),
                      area.overlays.end(),
                      [](const AreaOverlayStyle& overlay) { return overlay.enabled; }));
  }
  scene->stats.line_command_count = static_cast<uint64_t>(scene->lines.size());
  scene->stats.point_command_count = static_cast<uint64_t>(scene->points.size());
  scene->stats.label_candidate_count = static_cast<uint64_t>(scene->labels.size());
}

std::string_view FindAttributeValue(const AttributeList& attributes, std::string_view code) {
  for (const auto& [key, value] : attributes) {
    if (key == code) {
      return value;
    }
  }
  return {};
}

std::optional<double> ParseDoubleAttribute(const AttributeList& attributes,
                                           std::string_view code) {
  const std::string_view value = FindAttributeValue(attributes, code);
  if (value.empty()) {
    return std::nullopt;
  }

  double parsed = 0.0;
  const auto* begin = value.data();
  const auto* end = value.data() + value.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec == std::errc{} && result.ptr == end) {
    return parsed;
  }

  try {
    return std::stod(std::string(value));
  } catch (...) {
    return std::nullopt;
  }
}

bool IsMetaObjectClass(std::string_view object_class_acronym) {
  return object_class_acronym.size() >= 2 && object_class_acronym[0] == 'M' &&
         object_class_acronym[1] == '_';
}

bool IsQualityOfDataObject(std::string_view object_class_acronym) {
  return object_class_acronym == "M_QUAL";
}

bool HasEnabledAreaOverlay(const std::vector<AreaOverlayStyle>& overlays) {
  return std::any_of(overlays.begin(),
                     overlays.end(),
                     [](const AreaOverlayStyle& overlay) { return overlay.enabled; });
}

bool IsLightObjectClass(std::string_view object_class_acronym) {
  constexpr std::array<std::string_view, 4> kLightClasses = {
      "LIGHTS", "LITVES", "BOYLAT", "BCNLAT"};
  for (std::string_view candidate : kLightClasses) {
    if (candidate == object_class_acronym) {
      return true;
    }
  }
  return false;
}

bool IsStandaloneLightObjectClass(std::string_view object_class_acronym) {
  return object_class_acronym == "LIGHTS" || object_class_acronym == "LITVES";
}

bool IsNavigationMarkObjectClass(std::string_view object_class_acronym) {
  constexpr std::array<std::string_view, 11> kNavigationMarkClasses = {
      "LIGHTS", "LITVES", "BOYLAT", "BCNLAT", "BCNCAR", "BCNISD",
      "BCNSAW", "BCNSPP", "BOYCAR", "BOYISD", "BOYSAW"};
  for (std::string_view candidate : kNavigationMarkClasses) {
    if (candidate == object_class_acronym) {
      return true;
    }
  }
  return false;
}

bool IsBeaconObjectClass(std::string_view object_class_acronym) {
  return object_class_acronym == "BCNCAR" || object_class_acronym == "BCNISD" ||
         object_class_acronym == "BCNLAT" || object_class_acronym == "BCNSAW" ||
         object_class_acronym == "BCNSPP";
}

bool IsBuoyObjectClass(std::string_view object_class_acronym) {
  return object_class_acronym == "BOYCAR" || object_class_acronym == "BOYISD" ||
         object_class_acronym == "BOYLAT" || object_class_acronym == "BOYSAW" ||
         object_class_acronym == "BOYSPP";
}

bool AttributeContainsIntValue(const AttributeList& attributes,
                               std::string_view code,
                               int expected_value) {
  const auto tokens = SplitAttributeValue(FindAttributeValue(attributes, code));
  for (const auto& token : tokens) {
    if (const auto parsed = ParseIntAttributeValue(token);
        parsed.has_value() && *parsed == expected_value) {
      return true;
    }
  }
  return false;
}

std::optional<int> FindFirstAttributeIntValue(const AttributeList& attributes,
                                              std::string_view code) {
  const auto tokens = SplitAttributeValue(FindAttributeValue(attributes, code));
  for (const auto& token : tokens) {
    if (const auto parsed = ParseIntAttributeValue(token); parsed.has_value()) {
      return parsed;
    }
  }
  return std::nullopt;
}

bool HasAnyDepthValue(const AttributeList& attributes) {
  return ParseDoubleAttribute(attributes, "DRVAL1").has_value() ||
         ParseDoubleAttribute(attributes, "DRVAL2").has_value();
}

bool HasLowAccuracyPositioning(const AttributeList& attributes) {
  const auto quapos = FindFirstAttributeIntValue(attributes, "QUAPOS");
  return quapos.has_value() && *quapos >= 2 && *quapos < 10;
}

AreaOverlayStyle MakeAreaOverlayStyle(AreaOverlayKind kind,
                                      AreaOverlayPlacement placement,
                                      std::string_view palette_id,
                                      int spacing_px,
                                      int line_width_px,
                                      const DisplaySettings& settings,
                                      const S57PortrayalCatalog& catalog) {
  return ResolveAreaOverlayStyle(AreaOverlayStyle{
                                     .kind = kind,
                                     .placement = placement,
                                     .color = {},
                                     .palette_color_id = std::string(palette_id),
                                     .spacing_px = spacing_px,
                                     .line_width_px = line_width_px,
                                     .enabled = true,
                                 },
                                 settings,
                                 catalog);
}

bool IsRestrictionAreaObjectClass(std::string_view object_class_acronym) {
  constexpr std::array<std::string_view, 3> kRestrictionAreaClasses = {
      "RESARE", "ACHARE", "PSSARE"};
  for (std::string_view candidate : kRestrictionAreaClasses) {
    if (candidate == object_class_acronym) {
      return true;
    }
  }
  return false;
}

bool PassesCommonFilters(std::string_view object_class_acronym,
                         const AttributeList& attributes,
                         const DisplaySettings& settings) {
  if (!settings.show_soundings && object_class_acronym == "SOUNDG") {
    return false;
  }
  if (!settings.show_lights && IsLightObjectClass(object_class_acronym)) {
    return false;
  }
  if (IsMetaObjectClass(object_class_acronym) && !settings.show_meta) {
    return false;
  }
  if (IsQualityOfDataObject(object_class_acronym) && !settings.show_quality_of_data) {
    return false;
  }

  const auto scamin = ParseDoubleAttribute(attributes, "SCAMIN");
  if (scamin.has_value() && settings.estimated_display_scale > 0.0 &&
      settings.estimated_display_scale > *scamin) {
    return false;
  }

  return true;
}

std::string_view ResolveDerivedAreaStyleId(const render::PolygonPrimitive& polygon,
                                           const DisplaySettings& settings,
                                           std::string_view fallback_style_id) {
  const std::string_view object_class = polygon.object_class_acronym;
  if (object_class == "SEAARE") {
    return "transparent_area";
  }
  if (object_class == "SBDARE") {
    return "label_only_area";
  }
  if (object_class == "SLCONS") {
    return "construction_area";
  }
  if (object_class == "LAKARE") {
    return "inland_water_area";
  }
  if (object_class == "TIDEWY") {
    return "label_only_area";
  }
  if (IsRestrictionAreaObjectClass(object_class)) {
    return "restriction_area";
  }
  if (object_class != "DEPARE" && object_class != "DRGARE" && object_class != "UNSARE") {
    return fallback_style_id;
  }

  const auto shallow = settings.shallow_contour_m;
  const auto safety = settings.safety_contour_m;
  const auto deep = settings.deep_contour_m;

  const auto depth_1 = ParseDoubleAttribute(polygon.attributes, "DRVAL1");
  const auto depth_2 = ParseDoubleAttribute(polygon.attributes, "DRVAL2");
  if (!depth_1.has_value() && !depth_2.has_value()) {
    return fallback_style_id;
  }

  double drval1 = depth_1.value_or(depth_2.value_or(0.0));
  double drval2 = depth_2.value_or(depth_1.value_or(drval1));
  if (drval1 > drval2) {
    std::swap(drval1, drval2);
  }

  std::string_view style_id = "depth_intertidal_area";
  if (drval1 >= 0.0 && drval2 > 0.0) {
    style_id = "depth_shallow_area";
  }
  if (drval1 >= shallow && drval2 > shallow) {
    style_id = "depth_safe_area";
  }
  if (drval1 >= safety && drval2 > safety) {
    style_id = "depth_deep_area";
  }
  if (drval1 >= deep && drval2 > deep) {
    style_id = "depth_very_deep_area";
  }
  return style_id;
}

std::string_view ResolveDerivedLineStyleId(const render::PolylinePrimitive& polyline,
                                           const DisplaySettings& settings,
                                           std::string_view fallback_style_id) {
  if (!settings.symbolized_boundaries &&
      (polyline.object_class_acronym == "COALNE" ||
       polyline.object_class_acronym == "FAIRWY" ||
       polyline.object_class_acronym == "NAVLNE")) {
    return "plain_boundary";
  }

  if (polyline.object_class_acronym != "DEPCNT") {
    return fallback_style_id;
  }

  const auto contour = ParseDoubleAttribute(polyline.attributes, "VALDCO");
  if (!contour.has_value()) {
    return fallback_style_id;
  }

  if (std::abs(*contour - settings.safety_contour_m) <= 1e-3) {
    return "depth_contour_safety";
  }
  return fallback_style_id;
}

std::string_view ResolveDerivedPointStyleId(const render::PointPrimitive& point,
                                            const DisplaySettings& settings,
                                            std::string_view fallback_style_id) {
  if (settings.simplified_points && IsNavigationMarkObjectClass(point.object_class_acronym)) {
    return "default_point";
  }
  return fallback_style_id;
}

}  // namespace navscene::portrayal::detail
