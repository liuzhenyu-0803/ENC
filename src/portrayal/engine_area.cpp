#include "portrayal/engine_internal.h"
#include "portrayal/rule_program_internal.h"

namespace navscene::portrayal::detail {

void ResolveProceduralAreaStyle(const render::PolygonPrimitive& polygon,
                                FillStyle* fill,
                                StrokeStyle* stroke,
                                const DisplaySettings& settings,
                                const S57PortrayalCatalog& catalog) {
  if (fill == nullptr || stroke == nullptr) {
    return;
  }

  if (ApplyAreaRuleProgram(polygon, fill, stroke, settings, catalog)) {
    return;
  }

  auto set_fill_palette = [&](std::string_view palette_id) {
    fill->palette_color_id = std::string(palette_id);
    fill->color = ResolvePaletteColor(catalog, settings, palette_id, fill->color);
  };
  auto set_stroke_palette = [&](std::string_view palette_id) {
    stroke->palette_color_id = std::string(palette_id);
    stroke->color = ResolvePaletteColor(catalog, settings, palette_id, stroke->color);
  };
  const Rgb8 background_color = ResolveBackgroundColor(settings, catalog);

  if (polygon.object_class_acronym == "OBSTRN") {
    const auto valsou = ParseDoubleAttribute(polygon.attributes, "VALSOU");
    const auto watlev = FindFirstAttributeIntValue(polygon.attributes, "WATLEV");

    if (AttributeContainsIntValue(polygon.attributes, "CATOBS", 9)) {
      fill->enabled = false;
      stroke->enabled = true;
      stroke->pattern = StrokePatternKind::kDash;
      stroke->width_px = 1;
      set_stroke_palette("CHMGD");
      return;
    }
    if (AttributeContainsIntValue(polygon.attributes, "CATOBS", 7)) {
      fill->enabled = false;
      stroke->enabled = true;
      stroke->pattern = StrokePatternKind::kDash;
      stroke->width_px = 1;
      set_stroke_palette("CHGRD");
      return;
    }
    if (AttributeContainsIntValue(polygon.attributes, "CATOBS", 8) ||
        AttributeContainsIntValue(polygon.attributes, "CATOBS", 10) ||
        (watlev.has_value() && *watlev == 7)) {
      fill->enabled = false;
      stroke->enabled = true;
      stroke->pattern = StrokePatternKind::kDash;
      stroke->width_px = 1;
      set_stroke_palette("CSTLN");
      return;
    }
    if (AttributeContainsIntValue(polygon.attributes, "CATOBS", 6)) {
      fill->enabled = true;
      stroke->enabled = true;
      stroke->pattern = StrokePatternKind::kDot;
      stroke->width_px = 2;
      set_fill_palette("DEPIT");
      set_stroke_palette("CHBLK");
      return;
    }
    fill->enabled = true;
    stroke->enabled = true;
    stroke->pattern = StrokePatternKind::kSolid;
    stroke->width_px = 1;
    if ((watlev.has_value() && *watlev == 4) || (valsou.has_value() && *valsou <= 0.0)) {
      set_fill_palette("DEPVS");
      set_stroke_palette("CHBLK");
    } else if (watlev.has_value() && *watlev == 5) {
      set_fill_palette("DEPIT");
      set_stroke_palette("CHBLK");
    } else {
      set_fill_palette("DEPMS");
      set_stroke_palette("CSTLN");
    }
    return;
  }

  if (polygon.object_class_acronym == "M_QUAL") {
    fill->enabled = false;
    stroke->enabled = true;
    stroke->pattern = StrokePatternKind::kDash;
    stroke->width_px = 2;
    set_stroke_palette("CHGRD");
    return;
  }

  if (polygon.object_class_acronym == "SBDARE") {
    fill->enabled = false;
    stroke->enabled = false;

    const auto watlev = FindFirstAttributeIntValue(polygon.attributes, "WATLEV");
    if (watlev.has_value() && (*watlev == 3 || *watlev == 4)) {
      stroke->enabled = true;
      stroke->pattern = StrokePatternKind::kDash;
      stroke->width_px = 1;
      set_stroke_palette("CHGRD");
    }
    return;
  }

  if (polygon.object_class_acronym == "CBLARE") {
    fill->enabled = false;
    stroke->enabled = true;
    stroke->pattern = StrokePatternKind::kDash;
    stroke->width_px = 2;
    set_stroke_palette("CHMGD");
    return;
  }

  if (polygon.object_class_acronym == "AIRARE") {
    const bool conspicuous = AttributeContainsIntValue(polygon.attributes, "CONVIS", 1);
    fill->enabled = conspicuous;
    stroke->enabled = true;
    stroke->pattern = StrokePatternKind::kSolid;
    stroke->width_px = 1;
    if (conspicuous) {
      set_fill_palette("LANDA");
    }
    set_stroke_palette(conspicuous ? "CHBLK" : "LANDF");
    return;
  }

  if (polygon.object_class_acronym == "UNSARE") {
    fill->enabled = true;
    stroke->enabled = true;
    stroke->pattern = StrokePatternKind::kSolid;
    stroke->width_px = 2;
    set_fill_palette("NODTA");
    set_stroke_palette("CHGRD");
    return;
  }

  if (polygon.object_class_acronym == "VEGATN") {
    fill->enabled = false;
    stroke->enabled = true;
    stroke->pattern = StrokePatternKind::kDash;
    stroke->width_px = 1;
    set_stroke_palette("LANDF");
    return;
  }

  if (polygon.object_class_acronym == "ADMARE") {
    fill->enabled = false;
    stroke->enabled = true;
    stroke->pattern = StrokePatternKind::kDash;
    stroke->width_px = 2;
    set_stroke_palette("CHGRF");
    return;
  }

  if (polygon.object_class_acronym == "BRIDGE") {
    fill->enabled = false;
    stroke->enabled = true;
    stroke->pattern = StrokePatternKind::kSolid;
    stroke->width_px = 4;
    set_stroke_palette("CHGRD");
    return;
  }

  if (polygon.object_class_acronym == "TSEZNE") {
    fill->enabled = true;
    stroke->enabled = false;
    set_fill_palette("TRFCF");
    fill->color = MixColors(fill->color, background_color, 0.18);
    fill->palette_color_id.clear();
    return;
  }

  if (!IsRestrictionAreaObjectClass(polygon.object_class_acronym)) {
    return;
  }

  if (polygon.object_class_acronym == "ACHARE") {
    fill->enabled = false;
    stroke->enabled = true;
    stroke->pattern = StrokePatternKind::kDash;
    stroke->width_px = 2;
    set_stroke_palette("CHMGF");
    return;
  }

  if (polygon.object_class_acronym == "PSSARE") {
    fill->enabled = false;
    stroke->enabled = true;
    stroke->pattern = StrokePatternKind::kSolid;
    stroke->width_px = 2;
    set_stroke_palette("CHMGD");
    return;
  }

  const bool is_emphasized_special_use =
      AttributeContainsIntValue(polygon.attributes, "CATREA", 4) ||
      AttributeContainsIntValue(polygon.attributes, "CATREA", 10) ||
      AttributeContainsIntValue(polygon.attributes, "RESTRN", 14);
  const bool is_symbolized_boundary =
      AttributeContainsIntValue(polygon.attributes, "CATREA", 27) ||
      AttributeContainsIntValue(polygon.attributes, "CATREA", 28);

  if (is_symbolized_boundary) {
    fill->enabled = false;
    stroke->enabled = true;
    stroke->pattern = StrokePatternKind::kSolid;
    stroke->width_px = 2;
    set_stroke_palette("CHMGD");
    return;
  }

  if (is_emphasized_special_use) {
    fill->enabled = true;
    stroke->enabled = true;
    stroke->pattern = StrokePatternKind::kDash;
    stroke->width_px = 2;
    set_fill_palette("CHMGF");
    set_stroke_palette("CHMGD");
  }
}

std::vector<AreaOverlayStyle> ResolveProceduralAreaOverlays(
    const render::PolygonPrimitive& polygon,
    const DisplaySettings& settings,
    const S57PortrayalCatalog& catalog) {
  bool handled = false;
  std::vector<AreaOverlayStyle> overlays =
      ResolveAreaRuleProgramOverlays(polygon, settings, catalog, &handled);
  if (handled) {
    return overlays;
  }

  const auto watlev = FindFirstAttributeIntValue(polygon.attributes, "WATLEV");

  if (polygon.object_class_acronym == "AIRARE") {
    return {MakeAreaOverlayStyle(AreaOverlayKind::kAirport,
                                 AreaOverlayPlacement::kTileInArea,
                                 "LANDF",
                                 78,
                                 1,
                                 settings,
                                 catalog)};
  }

  if (polygon.object_class_acronym == "CBLARE") {
    return {MakeAreaOverlayStyle(AreaOverlayKind::kCableBoundary,
                                 AreaOverlayPlacement::kAlongBoundary,
                                 "CHMGF",
                                 88,
                                 2,
                                 settings,
                                 catalog)};
  }

  if (polygon.object_class_acronym == "SBDARE" &&
      watlev.has_value() && *watlev == 4 &&
      (AttributeContainsIntValue(polygon.attributes, "NATSUR", 9) ||
       AttributeContainsIntValue(polygon.attributes, "NATSUR", 11) ||
       AttributeContainsIntValue(polygon.attributes, "NATSUR", 14))) {
    return {MakeAreaOverlayStyle(AreaOverlayKind::kRockLedge,
                                 AreaOverlayPlacement::kTileInArea,
                                 "LANDF",
                                 90,
                                 1,
                                 settings,
                                 catalog)};
  }

  if (polygon.object_class_acronym == "UNSARE") {
    return {MakeAreaOverlayStyle(AreaOverlayKind::kNoDataArea,
                                 AreaOverlayPlacement::kTileInArea,
                                 "CHGRD",
                                 56,
                                 2,
                                 settings,
                                 catalog)};
  }

  if (polygon.object_class_acronym == "VEGATN") {
    const bool mangrove = AttributeContainsIntValue(polygon.attributes, "CATVEG", 7) ||
                          AttributeContainsIntValue(polygon.attributes, "CATVEG", 21);
    return {MakeAreaOverlayStyle(mangrove ? AreaOverlayKind::kVegetationMangrove
                                          : AreaOverlayKind::kVegetationWooded,
                                 AreaOverlayPlacement::kTileInArea,
                                 "LANDF",
                                 mangrove ? 72 : 64,
                                 1,
                                 settings,
                                 catalog)};
  }

  if (polygon.object_class_acronym != "M_QUAL") {
    return {};
  }

  AreaOverlayKind overlay_kind = AreaOverlayKind::kNoData;
  if (AttributeContainsIntValue(polygon.attributes, "CATZOC", 1)) {
    overlay_kind = AreaOverlayKind::kDataQualityA1;
  } else if (AttributeContainsIntValue(polygon.attributes, "CATZOC", 2)) {
    overlay_kind = AreaOverlayKind::kDataQualityA2;
  } else if (AttributeContainsIntValue(polygon.attributes, "CATZOC", 3)) {
    overlay_kind = AreaOverlayKind::kDataQualityB;
  } else if (AttributeContainsIntValue(polygon.attributes, "CATZOC", 4)) {
    overlay_kind = AreaOverlayKind::kDataQualityC;
  } else if (AttributeContainsIntValue(polygon.attributes, "CATZOC", 5)) {
    overlay_kind = AreaOverlayKind::kDataQualityD;
  } else if (AttributeContainsIntValue(polygon.attributes, "CATZOC", 6)) {
    overlay_kind = AreaOverlayKind::kDataQualityUnknown;
  }

  return {MakeAreaOverlayStyle(overlay_kind,
                               AreaOverlayPlacement::kTileInArea,
                               "CHGRD",
                               86,
                               2,
                               settings,
                               catalog)};
}

}  // namespace navscene::portrayal::detail
