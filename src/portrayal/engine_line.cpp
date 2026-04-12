#include "portrayal/engine_internal.h"
#include "portrayal/rule_program_internal.h"

namespace navscene::portrayal::detail {

void ResolveProceduralLineStyle(const render::PolylinePrimitive& polyline,
                                StrokeStyle* stroke,
                                const DisplaySettings& settings,
                                const S57PortrayalCatalog& catalog) {
  if (stroke == nullptr) {
    return;
  }

  if (ApplyLineRuleProgram(polyline, stroke, settings, catalog)) {
    return;
  }

  auto set_stroke_palette = [&](std::string_view palette_id) {
    stroke->palette_color_id = std::string(palette_id);
    stroke->color = ResolvePaletteColor(catalog, settings, palette_id, stroke->color);
  };

  if (polyline.object_class_acronym == "BRIDGE") {
    stroke->pattern = StrokePatternKind::kSolid;
    stroke->width_px = 4;
    set_stroke_palette("CHGRD");
    return;
  }

  if (polyline.object_class_acronym != "OBSTRN") {
    return;
  }

  if (AttributeContainsIntValue(polyline.attributes, "CATOBS", 9)) {
    stroke->pattern = StrokePatternKind::kDash;
    stroke->width_px = 1;
    set_stroke_palette("CHMGD");
    return;
  }
  if (AttributeContainsIntValue(polyline.attributes, "CATOBS", 7)) {
    stroke->pattern = StrokePatternKind::kDash;
    stroke->width_px = 1;
    set_stroke_palette("CHGRD");
    return;
  }
  if (AttributeContainsIntValue(polyline.attributes, "CATOBS", 8) ||
      AttributeContainsIntValue(polyline.attributes, "CATOBS", 10) ||
      AttributeContainsIntValue(polyline.attributes, "WATLEV", 7)) {
    stroke->pattern = StrokePatternKind::kDash;
    stroke->width_px = 1;
    set_stroke_palette("CSTLN");
    return;
  }
  if (AttributeContainsIntValue(polyline.attributes, "CATOBS", 6)) {
    stroke->pattern = StrokePatternKind::kDot;
    stroke->width_px = 2;
    set_stroke_palette("CHBLK");
  }
}

}  // namespace navscene::portrayal::detail
