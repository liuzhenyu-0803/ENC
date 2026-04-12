#include "portrayal/engine_internal.h"
#include "portrayal/rule_program_internal.h"

namespace navscene::portrayal::detail {
namespace {

std::optional<PointSymbolStyle> ResolveLowAccuracyPointDecorationStyle(
    const render::PointPrimitive& point,
    const DisplaySettings& settings,
    const S57PortrayalCatalog& catalog) {
  const auto quapos = FindFirstAttributeIntValue(point.attributes, "QUAPOS");
  if (!quapos.has_value() || *quapos < 2 || *quapos >= 10) {
    return std::nullopt;
  }

  PointSymbolStyle style{
      .kind = PointSymbolKind::kCircle,
      .fill = ResolvePaletteColor(catalog, settings, "CHWHT", {212, 234, 238}),
      .fill_palette_color_id = "CHWHT",
      .stroke = ResolvePaletteColor(catalog, settings, "CHBLK", {7, 7, 7}),
      .stroke_palette_color_id = "CHBLK",
      .size_px = 6,
      .enabled = true,
  };

  switch (*quapos) {
    case 4:
      style.kind = PointSymbolKind::kTriangle;
      style.size_px = 6;
      break;
    case 5:
      style.kind = PointSymbolKind::kSquare;
      style.size_px = 6;
      break;
    case 7:
    case 8:
      style.kind = PointSymbolKind::kDiamond;
      style.size_px = 7;
      break;
    default:
      style.kind = PointSymbolKind::kCircle;
      style.size_px = 5;
      break;
  }

  return style;
}

}  // namespace

PointSymbolStyle ResolveProceduralPointStyle(const render::PointPrimitive& point,
                                             PointSymbolStyle style,
                                             const DisplaySettings& settings,
                                             const S57PortrayalCatalog& catalog) {
  if (ApplyPointRuleProgram(point, &style, settings, catalog)) {
    return style;
  }

  auto set_point_fill_palette = [&](std::string_view palette_id) {
    style.fill_palette_color_id = std::string(palette_id);
    style.fill = ResolvePaletteColor(catalog, settings, palette_id, style.fill);
  };
  auto set_point_stroke_palette = [&](std::string_view palette_id) {
    style.stroke_palette_color_id = std::string(palette_id);
    style.stroke = ResolvePaletteColor(catalog, settings, palette_id, style.stroke);
  };

  if (point.object_class_acronym == "OBSTRN") {
    const auto valsou = ParseDoubleAttribute(point.attributes, "VALSOU");
    const auto watlev = FindFirstAttributeIntValue(point.attributes, "WATLEV");

    style.kind = PointSymbolKind::kDiamond;

    if (AttributeContainsIntValue(point.attributes, "CATOBS", 9)) {
      style.size_px = 8;
      set_point_fill_palette("CHMGF");
      set_point_stroke_palette("CHMGD");
      return style;
    }
    if (AttributeContainsIntValue(point.attributes, "CATOBS", 7)) {
      style.size_px = 8;
      set_point_fill_palette("CHGRF");
      set_point_stroke_palette("CHBLK");
      return style;
    }
    if (AttributeContainsIntValue(point.attributes, "CATOBS", 8) ||
        AttributeContainsIntValue(point.attributes, "CATOBS", 10) ||
        (watlev.has_value() && *watlev == 7)) {
      style.size_px = 9;
      set_point_fill_palette("CHWHT");
      set_point_stroke_palette("CSTLN");
      return style;
    }
    if (watlev.has_value() && *watlev == 5) {
      style.size_px = 8;
      set_point_fill_palette("DEPIT");
      set_point_stroke_palette("CHBLK");
      return style;
    }
    if ((watlev.has_value() && *watlev == 4) || (valsou.has_value() && *valsou <= 0.0)) {
      style.size_px = 9;
      set_point_fill_palette("CHBLK");
      set_point_stroke_palette("CHWHT");
      return style;
    }
    if (valsou.has_value() && *valsou <= settings.safety_depth_m) {
      style.size_px = 8;
      set_point_fill_palette("DEPVS");
      set_point_stroke_palette("CHBLK");
      return style;
    }
    return style;
  }

  if (point.object_class_acronym == "UWTROC") {
    const auto valsou = ParseDoubleAttribute(point.attributes, "VALSOU");
    const auto watlev = FindFirstAttributeIntValue(point.attributes, "WATLEV");

    style.kind = PointSymbolKind::kDiamond;
    if ((watlev.has_value() && (*watlev == 4 || *watlev == 5)) ||
        (valsou.has_value() && *valsou <= 0.0)) {
      style.size_px = 9;
      set_point_fill_palette("CHBLK");
      set_point_stroke_palette("CHWHT");
      return style;
    }
    if (valsou.has_value() && *valsou <= settings.safety_depth_m) {
      style.size_px = 9;
      set_point_fill_palette("DEPVS");
      set_point_stroke_palette("CHBLK");
      return style;
    }
    if (watlev.has_value() && *watlev == 3) {
      style.size_px = 8;
      set_point_fill_palette("DEPMS");
      set_point_stroke_palette("CSTLN");
      return style;
    }
    return style;
  }

  if (point.object_class_acronym == "WRECKS") {
    const auto valsou = ParseDoubleAttribute(point.attributes, "VALSOU");
    const auto watlev = FindFirstAttributeIntValue(point.attributes, "WATLEV");

    if (AttributeContainsIntValue(point.attributes, "CATWRK", 5) ||
        (watlev.has_value() && *watlev == 1)) {
      style.kind = PointSymbolKind::kSquare;
      style.size_px = 10;
      set_point_fill_palette("CHGRD");
      set_point_stroke_palette("CHBLK");
      return style;
    }
    if ((watlev.has_value() && *watlev == 4) ||
        (valsou.has_value() && *valsou <= settings.safety_depth_m)) {
      style.kind = PointSymbolKind::kDiamond;
      style.size_px = 9;
      set_point_fill_palette("DEPVS");
      set_point_stroke_palette("CHBLK");
      return style;
    }
    style.kind = PointSymbolKind::kDiamond;
    style.size_px = 8;
    set_point_fill_palette("CHWHT");
    set_point_stroke_palette("CHBLK");
    return style;
  }

  return style;
}

std::vector<PointCommand> ResolveProceduralPointDecorations(
    const render::PointPrimitive& point,
    int base_priority,
    const DisplaySettings& settings,
    const S57PortrayalCatalog& catalog) {
  std::vector<PointCommand> decorations;

  const auto low_accuracy_style =
      ResolveLowAccuracyPointDecorationStyle(point, settings, catalog);
  if (low_accuracy_style.has_value()) {
    decorations.push_back(PointCommand{
        .priority = base_priority + 1,
        .geometry = point,
        .symbol = std::move(*low_accuracy_style),
        .visible = true,
    });
  }

  return decorations;
}

}  // namespace navscene::portrayal::detail
