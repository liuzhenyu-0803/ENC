#include "portrayal/rule_program_internal.h"

namespace navscene::portrayal::detail {
namespace {

void SetPointFillPalette(PointSymbolStyle* style,
                         std::string_view palette_id,
                         const DisplaySettings& settings,
                         const S57PortrayalCatalog& catalog) {
  if (style == nullptr) {
    return;
  }
  style->fill_palette_color_id = std::string(palette_id);
  style->fill = ResolvePaletteColor(catalog, settings, palette_id, style->fill);
}

void SetPointStrokePalette(PointSymbolStyle* style,
                           std::string_view palette_id,
                           const DisplaySettings& settings,
                           const S57PortrayalCatalog& catalog) {
  if (style == nullptr) {
    return;
  }
  style->stroke_palette_color_id = std::string(palette_id);
  style->stroke = ResolvePaletteColor(catalog, settings, palette_id, style->stroke);
}

PointSymbolKind ResolveTopmarkPointKind(const AttributeList& attributes) {
  const auto topshp = ParseIntAttributeValue(FindAttributeValue(attributes, "TOPSHP"));
  if (!topshp.has_value()) {
    return PointSymbolKind::kTriangle;
  }

  switch (*topshp) {
    case 3:
    case 4:
    case 22:
      return PointSymbolKind::kCircle;
    case 12:
    case 18:
      return PointSymbolKind::kDiamond;
    case 19:
    case 24:
      return PointSymbolKind::kSquare;
    default:
      return PointSymbolKind::kTriangle;
  }
}

std::string_view ResolveTopmarkPaletteColorId(const AttributeList& attributes,
                                              bool fill_role) {
  if (AttributeContainsIntValue(attributes, "COLOUR", 11)) {
    if (AttributeContainsIntValue(attributes, "COLOUR", 6)) {
      return fill_role ? "CHYLW" : "CHBLK";
    }
    if (AttributeContainsIntValue(attributes, "COLOUR", 3)) {
      return fill_role ? "CHRED" : "CHBLK";
    }
    if (AttributeContainsIntValue(attributes, "COLOUR", 4)) {
      return fill_role ? "CHGRN" : "CHBLK";
    }
    if (AttributeContainsIntValue(attributes, "COLOUR", 1)) {
      return fill_role ? "CHWHT" : "CHBLK";
    }
  }

  if (AttributeContainsIntValue(attributes, "COLOUR", 1)) {
    if (AttributeContainsIntValue(attributes, "COLOUR", 3)) {
      return fill_role ? "CHWHT" : "CHRED";
    }
    if (AttributeContainsIntValue(attributes, "COLOUR", 4)) {
      return fill_role ? "CHWHT" : "CHGRN";
    }
    if (AttributeContainsIntValue(attributes, "COLOUR", 6)) {
      return fill_role ? "CHWHT" : "CHYLW";
    }
    return "CHWHT";
  }

  if (AttributeContainsIntValue(attributes, "COLOUR", 3)) {
    return fill_role ? "CHRED" : "CHBLK";
  }
  if (AttributeContainsIntValue(attributes, "COLOUR", 4)) {
    return fill_role ? "CHGRN" : "CHBLK";
  }
  if (AttributeContainsIntValue(attributes, "COLOUR", 6)) {
    return fill_role ? "CHYLW" : "CHBLK";
  }
  if (AttributeContainsIntValue(attributes, "COLOUR", 11)) {
    return fill_role ? "CHBLK" : "CHWHT";
  }
  return fill_role ? "CHMGD" : "CHBLK";
}

std::string_view ResolveLightPaletteColorId(const AttributeList& attributes,
                                            bool fill_role) {
  if (AttributeContainsIntValue(attributes, "COLOUR", 3)) {
    return fill_role ? "LITRD" : "CHBLK";
  }
  if (AttributeContainsIntValue(attributes, "COLOUR", 4)) {
    return fill_role ? "LITGN" : "CHBLK";
  }
  if (AttributeContainsIntValue(attributes, "COLOUR", 6)) {
    return fill_role ? "LITYW" : "CHBLK";
  }
  if (AttributeContainsIntValue(attributes, "COLOUR", 1)) {
    return fill_role ? "CHWHT" : "CHBLK";
  }
  if (AttributeContainsIntValue(attributes, "COLOUR", 11)) {
    return fill_role ? "CHMGD" : "CHBLK";
  }
  return fill_role ? "CHYLW" : "CHBRN";
}

std::optional<double> ResolveEstimatedObstructionDepth(const render::PointPrimitive& point) {
  if (const auto valsou = ParseDoubleAttribute(point.attributes, "VALSOU"); valsou.has_value()) {
    return valsou;
  }

  if (point.object_class_acronym == "OBSTRN") {
    if (AttributeContainsIntValue(point.attributes, "CATOBS", 6)) {
      return 0.01;
    }

    const auto watlev = FindFirstAttributeIntValue(point.attributes, "WATLEV");
    if (!watlev.has_value()) {
      return -15.0;
    }

    switch (*watlev) {
      case 5:
        return 0.0;
      case 3:
        return 0.01;
      case 1:
      case 2:
      case 4:
      default:
        return -15.0;
    }
  }

  if (point.object_class_acronym == "UWTROC") {
    const auto watlev = FindFirstAttributeIntValue(point.attributes, "WATLEV");
    if (!watlev.has_value()) {
      return std::nullopt;
    }

    switch (*watlev) {
      case 5:
        return 0.0;
      case 3:
        return 0.01;
      case 1:
      case 2:
      case 4:
      default:
        return -15.0;
    }
  }

  return std::nullopt;
}

std::optional<double> ResolveEstimatedWreckDepth(const render::PointPrimitive& point) {
  if (const auto valsou = ParseDoubleAttribute(point.attributes, "VALSOU"); valsou.has_value()) {
    return valsou;
  }

  const auto catwrk = FindFirstAttributeIntValue(point.attributes, "CATWRK");
  if (catwrk.has_value()) {
    switch (*catwrk) {
      case 1:
        return 20.0;
      case 2:
        return 0.0;
      case 4:
      case 5:
        return -15.0;
      default:
        break;
    }
  }

  const auto watlev = FindFirstAttributeIntValue(point.attributes, "WATLEV");
  if (!watlev.has_value()) {
    return -15.0;
  }

  switch (*watlev) {
    case 3:
      return 0.01;
    case 5:
      return 0.0;
    case 1:
    case 2:
    case 4:
    case 6:
    default:
      return -15.0;
  }
}

bool ApplyHazardPointRuleProgram(const render::PointPrimitive& point,
                                 PointSymbolStyle* style,
                                 const DisplaySettings& settings,
                                 const S57PortrayalCatalog& catalog) {
  if (style == nullptr) {
    return false;
  }

  if (point.object_class_acronym == "OBSTRN") {
    const auto watlev = FindFirstAttributeIntValue(point.attributes, "WATLEV");
    const auto estimated_depth = ResolveEstimatedObstructionDepth(point);

    style->kind = PointSymbolKind::kDiamond;

    if (AttributeContainsIntValue(point.attributes, "CATOBS", 9)) {
      style->size_px = 8;
      SetPointFillPalette(style, "CHMGF", settings, catalog);
      SetPointStrokePalette(style, "CHMGD", settings, catalog);
      return true;
    }
    if (AttributeContainsIntValue(point.attributes, "CATOBS", 7)) {
      style->size_px = 8;
      SetPointFillPalette(style, "CHGRF", settings, catalog);
      SetPointStrokePalette(style, "CHBLK", settings, catalog);
      return true;
    }
    if (AttributeContainsIntValue(point.attributes, "CATOBS", 8) ||
        AttributeContainsIntValue(point.attributes, "CATOBS", 10) ||
        (watlev.has_value() && *watlev == 7)) {
      style->size_px = 9;
      SetPointFillPalette(style, "CHWHT", settings, catalog);
      SetPointStrokePalette(style, "CSTLN", settings, catalog);
      return true;
    }
    if (watlev.has_value() && *watlev == 5) {
      style->size_px = 8;
      SetPointFillPalette(style, "DEPIT", settings, catalog);
      SetPointStrokePalette(style, "CHBLK", settings, catalog);
      return true;
    }
    if ((watlev.has_value() && *watlev == 4) ||
        (estimated_depth.has_value() && *estimated_depth <= 0.0)) {
      style->size_px = 9;
      SetPointFillPalette(style, "CHBLK", settings, catalog);
      SetPointStrokePalette(style, "CHWHT", settings, catalog);
      return true;
    }
    if (estimated_depth.has_value() && *estimated_depth <= settings.safety_contour_m) {
      style->size_px = 8;
      SetPointFillPalette(style, "DEPVS", settings, catalog);
      SetPointStrokePalette(style, "CHBLK", settings, catalog);
      return true;
    }
    return true;
  }

  if (point.object_class_acronym == "UWTROC") {
    const auto watlev = FindFirstAttributeIntValue(point.attributes, "WATLEV");
    const auto estimated_depth = ResolveEstimatedObstructionDepth(point);

    style->kind = PointSymbolKind::kDiamond;
    if ((watlev.has_value() && (*watlev == 4 || *watlev == 5)) ||
        (estimated_depth.has_value() && *estimated_depth <= 0.0)) {
      style->size_px = 9;
      SetPointFillPalette(style, "CHBLK", settings, catalog);
      SetPointStrokePalette(style, "CHWHT", settings, catalog);
      return true;
    }
    if (estimated_depth.has_value() && *estimated_depth <= settings.safety_contour_m) {
      style->size_px = 9;
      SetPointFillPalette(style, "DEPVS", settings, catalog);
      SetPointStrokePalette(style, "CHBLK", settings, catalog);
      return true;
    }
    if (watlev.has_value() && *watlev == 3) {
      style->size_px = 8;
      SetPointFillPalette(style, "DEPMS", settings, catalog);
      SetPointStrokePalette(style, "CSTLN", settings, catalog);
      return true;
    }
    return true;
  }

  if (point.object_class_acronym == "WRECKS") {
    const auto watlev = FindFirstAttributeIntValue(point.attributes, "WATLEV");
    const auto catwrk = FindFirstAttributeIntValue(point.attributes, "CATWRK");
    const auto estimated_depth = ResolveEstimatedWreckDepth(point);

    if ((catwrk.has_value() && (*catwrk == 4 || *catwrk == 5)) ||
        (watlev.has_value() && (*watlev == 1 || *watlev == 2))) {
      style->kind = PointSymbolKind::kSquare;
      style->size_px = 10;
      SetPointFillPalette(style, "CHGRD", settings, catalog);
      SetPointStrokePalette(style, "CHBLK", settings, catalog);
      return true;
    }

    style->kind = PointSymbolKind::kDiamond;
    if ((watlev.has_value() && *watlev == 4) ||
        (estimated_depth.has_value() && *estimated_depth <= settings.safety_contour_m)) {
      style->size_px = 9;
      SetPointFillPalette(style, "DEPVS", settings, catalog);
      SetPointStrokePalette(style, "CHBLK", settings, catalog);
      return true;
    }

    style->size_px = 8;
    SetPointFillPalette(style, "CHWHT", settings, catalog);
    SetPointStrokePalette(style, "CHBLK", settings, catalog);
    return true;
  }

  return false;
}

bool ApplyTopmarkPointRuleProgram(const render::PointPrimitive& point,
                                  PointSymbolStyle* style,
                                  const DisplaySettings& settings,
                                  const S57PortrayalCatalog& catalog) {
  if (style == nullptr || point.object_class_acronym != "TOPMAR") {
    return false;
  }

  style->kind = ResolveTopmarkPointKind(point.attributes);
  SetPointFillPalette(
      style, ResolveTopmarkPaletteColorId(point.attributes, true), settings, catalog);
  SetPointStrokePalette(
      style, ResolveTopmarkPaletteColorId(point.attributes, false), settings, catalog);
  return true;
}

bool ApplyLightPointRuleProgram(const render::PointPrimitive& point,
                                PointSymbolStyle* style,
                                const DisplaySettings& settings,
                                const S57PortrayalCatalog& catalog) {
  if (style == nullptr ||
      (point.object_class_acronym != "LIGHTS" &&
       point.object_class_acronym != "LITVES")) {
    return false;
  }

  style->kind = PointSymbolKind::kDiamond;
  style->size_px = point.object_class_acronym == "LITVES" ? 8 : 7;

  if (AttributeContainsIntValue(point.attributes, "CATLIT", 1) ||
      AttributeContainsIntValue(point.attributes, "CATLIT", 16)) {
    style->size_px += 1;
  }
  if (AttributeContainsIntValue(point.attributes, "CATLIT", 9)) {
    style->kind = PointSymbolKind::kSquare;
  }

  SetPointFillPalette(
      style, ResolveLightPaletteColorId(point.attributes, true), settings, catalog);
  SetPointStrokePalette(
      style, ResolveLightPaletteColorId(point.attributes, false), settings, catalog);
  return true;
}

}  // namespace

bool ApplyPointRuleProgram(const render::PointPrimitive& point,
                           PointSymbolStyle* style,
                           const DisplaySettings& settings,
                           const S57PortrayalCatalog& catalog) {
  if (ApplyTopmarkPointRuleProgram(point, style, settings, catalog)) {
    return true;
  }
  if (ApplyLightPointRuleProgram(point, style, settings, catalog)) {
    return true;
  }
  return ApplyHazardPointRuleProgram(point, style, settings, catalog);
}

}  // namespace navscene::portrayal::detail
