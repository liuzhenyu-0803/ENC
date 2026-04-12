#include "portrayal/rule_program_internal.h"

namespace navscene::portrayal::detail {
namespace {

struct AreaExecutionState {
  FillStyle fill;
  StrokeStyle stroke;
  std::vector<AreaOverlayStyle> overlays;
};

void SetAreaFillPalette(AreaExecutionState* state,
                        std::string_view palette_id,
                        const DisplaySettings& settings,
                        const S57PortrayalCatalog& catalog) {
  if (state == nullptr) {
    return;
  }
  state->fill.palette_color_id = std::string(palette_id);
  state->fill.color =
      ResolvePaletteColor(catalog, settings, palette_id, state->fill.color);
}

void SetAreaStrokePalette(AreaExecutionState* state,
                          std::string_view palette_id,
                          const DisplaySettings& settings,
                          const S57PortrayalCatalog& catalog) {
  if (state == nullptr) {
    return;
  }
  state->stroke.palette_color_id = std::string(palette_id);
  state->stroke.color =
      ResolvePaletteColor(catalog, settings, palette_id, state->stroke.color);
}

bool MatchesCondition(const RuleCondition& condition, const AttributeList& attributes) {
  switch (condition.kind) {
    case RuleConditionKind::kAttributeIntContains:
      return AttributeContainsIntValue(attributes, condition.attribute_code, condition.int_value);
    case RuleConditionKind::kAttributeIntFirstEquals: {
      const auto value = FindFirstAttributeIntValue(attributes, condition.attribute_code);
      return value.has_value() && *value == condition.int_value;
    }
    case RuleConditionKind::kHasAnyDepthValue:
      return HasAnyDepthValue(attributes);
    case RuleConditionKind::kHasLowAccuracyPositioning:
      return HasLowAccuracyPositioning(attributes);
  }
  return false;
}

bool MatchesAllConditions(const std::vector<RuleCondition>& conditions,
                          const AttributeList& attributes) {
  for (const auto& condition : conditions) {
    if (!MatchesCondition(condition, attributes)) {
      return false;
    }
  }
  return true;
}

void ApplyInstructions(const std::vector<RuleInstruction>& instructions,
                       AreaExecutionState* state,
                       const DisplaySettings& settings,
                       const S57PortrayalCatalog& catalog) {
  if (state == nullptr) {
    return;
  }

  for (const auto& instruction : instructions) {
    switch (instruction.kind) {
      case RuleInstructionKind::kSetFillEnabled:
        state->fill.enabled = instruction.bool_value;
        break;
      case RuleInstructionKind::kSetStrokeEnabled:
        state->stroke.enabled = instruction.bool_value;
        break;
      case RuleInstructionKind::kSetFillPalette:
        state->fill.palette_color_id = instruction.palette_id;
        state->fill.color = ResolvePaletteColor(
            catalog, settings, instruction.palette_id, state->fill.color);
        break;
      case RuleInstructionKind::kSetStrokePalette:
        state->stroke.palette_color_id = instruction.palette_id;
        state->stroke.color = ResolvePaletteColor(
            catalog, settings, instruction.palette_id, state->stroke.color);
        break;
      case RuleInstructionKind::kSetStrokePattern:
        state->stroke.pattern = instruction.stroke_pattern;
        break;
      case RuleInstructionKind::kSetStrokeWidth:
        state->stroke.width_px = instruction.int_value;
        break;
      case RuleInstructionKind::kMixFillWithBackground:
        state->fill.color = MixColors(state->fill.color,
                                      ResolveBackgroundColor(settings, catalog),
                                      instruction.scalar_value);
        break;
      case RuleInstructionKind::kClearFillPalette:
        state->fill.palette_color_id.clear();
        break;
      case RuleInstructionKind::kAddAreaOverlay:
        state->overlays.push_back(MakeAreaOverlayStyle(instruction.overlay_kind,
                                                       instruction.overlay_placement,
                                                       instruction.palette_id,
                                                       instruction.overlay_spacing_px,
                                                       instruction.overlay_line_width_px,
                                                       settings,
                                                       catalog));
        break;
    }
  }
}

const RuleProgram* FindAreaRuleProgram(std::string_view object_class_acronym) {
  static const std::vector<RuleProgram> kPrograms = {
      RuleProgram{
          .geometry_kind = GeometryKind::kArea,
          .object_class_acronym = "AIRARE",
          .prefix =
              {
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeEnabled, .bool_value = true},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kSolid},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 1},
                  RuleInstruction{
                      .kind = RuleInstructionKind::kAddAreaOverlay,
                      .palette_id = "LANDF",
                      .overlay_kind = AreaOverlayKind::kAirport,
                      .overlay_placement = AreaOverlayPlacement::kTileInArea,
                      .overlay_spacing_px = 78,
                      .overlay_line_width_px = 1,
                  },
              },
          .branches =
              {
                  RuleBranch{
                      .conditions =
                          {
                              RuleCondition{
                                  .kind = RuleConditionKind::kAttributeIntContains,
                                  .attribute_code = "CONVIS",
                                  .int_value = 1,
                              },
                          },
                      .instructions =
                          {
                              RuleInstruction{.kind = RuleInstructionKind::kSetFillEnabled, .bool_value = true},
                              RuleInstruction{.kind = RuleInstructionKind::kSetFillPalette, .palette_id = "LANDA"},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CHBLK"},
                          },
                  },
              },
          .fallback =
              {
                  RuleInstruction{.kind = RuleInstructionKind::kSetFillEnabled, .bool_value = false},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "LANDF"},
              },
      },
      RuleProgram{
          .geometry_kind = GeometryKind::kArea,
          .object_class_acronym = "CBLARE",
          .prefix =
              {
                  RuleInstruction{.kind = RuleInstructionKind::kSetFillEnabled, .bool_value = false},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeEnabled, .bool_value = true},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kDash},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 2},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CHMGD"},
                  RuleInstruction{
                      .kind = RuleInstructionKind::kAddAreaOverlay,
                      .palette_id = "CHMGF",
                      .overlay_kind = AreaOverlayKind::kCableBoundary,
                      .overlay_placement = AreaOverlayPlacement::kAlongBoundary,
                      .overlay_spacing_px = 88,
                      .overlay_line_width_px = 2,
                  },
              },
      },
      RuleProgram{
          .geometry_kind = GeometryKind::kArea,
          .object_class_acronym = "UNSARE",
          .prefix =
              {
                  RuleInstruction{.kind = RuleInstructionKind::kSetFillEnabled, .bool_value = true},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeEnabled, .bool_value = true},
                  RuleInstruction{.kind = RuleInstructionKind::kSetFillPalette, .palette_id = "NODTA"},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CHGRD"},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kSolid},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 2},
                  RuleInstruction{
                      .kind = RuleInstructionKind::kAddAreaOverlay,
                      .palette_id = "CHGRD",
                      .overlay_kind = AreaOverlayKind::kNoDataArea,
                      .overlay_placement = AreaOverlayPlacement::kTileInArea,
                      .overlay_spacing_px = 56,
                      .overlay_line_width_px = 2,
                  },
              },
      },
      RuleProgram{
          .geometry_kind = GeometryKind::kArea,
          .object_class_acronym = "ADMARE",
          .prefix =
              {
                  RuleInstruction{.kind = RuleInstructionKind::kSetFillEnabled, .bool_value = false},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeEnabled, .bool_value = true},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kDash},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 2},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CHGRF"},
              },
      },
      RuleProgram{
          .geometry_kind = GeometryKind::kArea,
          .object_class_acronym = "BRIDGE",
          .prefix =
              {
                  RuleInstruction{.kind = RuleInstructionKind::kSetFillEnabled, .bool_value = false},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeEnabled, .bool_value = true},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kSolid},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 4},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CHGRD"},
              },
      },
      RuleProgram{
          .geometry_kind = GeometryKind::kArea,
          .object_class_acronym = "SLCONS",
          .prefix =
              {
                  RuleInstruction{.kind = RuleInstructionKind::kSetFillEnabled, .bool_value = false},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeEnabled, .bool_value = true},
              },
          .branches =
              {
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kHasLowAccuracyPositioning}},
                      .instructions =
                          {
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kDot},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 2},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CSTLN"},
                          },
                  },
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kAttributeIntContains, .attribute_code = "CONDTN", .int_value = 1}},
                      .instructions =
                          {
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kDash},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 1},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CSTLN"},
                          },
                  },
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kAttributeIntContains, .attribute_code = "CONDTN", .int_value = 2}},
                      .instructions =
                          {
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kDash},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 1},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CSTLN"},
                          },
                  },
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kAttributeIntFirstEquals, .attribute_code = "CATSLC", .int_value = 6}},
                      .instructions =
                          {
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kSolid},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 4},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CSTLN"},
                          },
                  },
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kAttributeIntFirstEquals, .attribute_code = "CATSLC", .int_value = 15}},
                      .instructions =
                          {
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kSolid},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 4},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CSTLN"},
                          },
                  },
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kAttributeIntFirstEquals, .attribute_code = "CATSLC", .int_value = 16}},
                      .instructions =
                          {
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kSolid},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 4},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CSTLN"},
                          },
                  },
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kAttributeIntFirstEquals, .attribute_code = "WATLEV", .int_value = 3}},
                      .instructions =
                          {
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kDash},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 2},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CSTLN"},
                          },
                  },
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kAttributeIntFirstEquals, .attribute_code = "WATLEV", .int_value = 4}},
                      .instructions =
                          {
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kDash},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 2},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CSTLN"},
                          },
                  },
              },
          .fallback =
              {
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kSolid},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 2},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CSTLN"},
              },
      },
      RuleProgram{
          .geometry_kind = GeometryKind::kArea,
          .object_class_acronym = "TSEZNE",
          .prefix =
              {
                  RuleInstruction{.kind = RuleInstructionKind::kSetFillEnabled, .bool_value = true},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeEnabled, .bool_value = false},
                  RuleInstruction{.kind = RuleInstructionKind::kSetFillPalette, .palette_id = "TRFCF"},
                  RuleInstruction{.kind = RuleInstructionKind::kMixFillWithBackground, .scalar_value = 0.18},
                  RuleInstruction{.kind = RuleInstructionKind::kClearFillPalette},
              },
      },
      RuleProgram{
          .geometry_kind = GeometryKind::kArea,
          .object_class_acronym = "ACHARE",
          .prefix =
              {
                  RuleInstruction{.kind = RuleInstructionKind::kSetFillEnabled, .bool_value = false},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeEnabled, .bool_value = true},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kDash},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 2},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CHMGF"},
              },
      },
      RuleProgram{
          .geometry_kind = GeometryKind::kArea,
          .object_class_acronym = "PSSARE",
          .prefix =
              {
                  RuleInstruction{.kind = RuleInstructionKind::kSetFillEnabled, .bool_value = false},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeEnabled, .bool_value = true},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kSolid},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 2},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CHMGD"},
              },
      },
      RuleProgram{
          .geometry_kind = GeometryKind::kArea,
          .object_class_acronym = "VEGATN",
          .prefix =
              {
                  RuleInstruction{.kind = RuleInstructionKind::kSetFillEnabled, .bool_value = false},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeEnabled, .bool_value = true},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kDash},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 1},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "LANDF"},
              },
          .branches =
              {
                  RuleBranch{
                      .conditions =
                          {
                              RuleCondition{
                                  .kind = RuleConditionKind::kAttributeIntContains,
                                  .attribute_code = "CATVEG",
                                  .int_value = 7,
                              },
                          },
                      .instructions =
                          {
                              RuleInstruction{
                                  .kind = RuleInstructionKind::kAddAreaOverlay,
                                  .palette_id = "LANDF",
                                  .overlay_kind = AreaOverlayKind::kVegetationMangrove,
                                  .overlay_placement = AreaOverlayPlacement::kTileInArea,
                                  .overlay_spacing_px = 72,
                                  .overlay_line_width_px = 1,
                              },
                          },
                  },
                  RuleBranch{
                      .conditions =
                          {
                              RuleCondition{
                                  .kind = RuleConditionKind::kAttributeIntContains,
                                  .attribute_code = "CATVEG",
                                  .int_value = 21,
                              },
                          },
                      .instructions =
                          {
                              RuleInstruction{
                                  .kind = RuleInstructionKind::kAddAreaOverlay,
                                  .palette_id = "LANDF",
                                  .overlay_kind = AreaOverlayKind::kVegetationMangrove,
                                  .overlay_placement = AreaOverlayPlacement::kTileInArea,
                                  .overlay_spacing_px = 72,
                                  .overlay_line_width_px = 1,
                              },
                          },
                  },
              },
          .fallback =
              {
                  RuleInstruction{
                      .kind = RuleInstructionKind::kAddAreaOverlay,
                      .palette_id = "LANDF",
                      .overlay_kind = AreaOverlayKind::kVegetationWooded,
                      .overlay_placement = AreaOverlayPlacement::kTileInArea,
                      .overlay_spacing_px = 64,
                      .overlay_line_width_px = 1,
                  },
              },
      },
      RuleProgram{
          .geometry_kind = GeometryKind::kArea,
          .object_class_acronym = "M_QUAL",
          .prefix =
              {
                  RuleInstruction{.kind = RuleInstructionKind::kSetFillEnabled, .bool_value = false},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeEnabled, .bool_value = true},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kDash},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 2},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CHGRD"},
              },
          .branches =
              {
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kAttributeIntContains, .attribute_code = "CATZOC", .int_value = 1}},
                      .instructions = {{.kind = RuleInstructionKind::kAddAreaOverlay, .palette_id = "CHGRD", .overlay_kind = AreaOverlayKind::kDataQualityA1, .overlay_placement = AreaOverlayPlacement::kTileInArea, .overlay_spacing_px = 86, .overlay_line_width_px = 2}},
                  },
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kAttributeIntContains, .attribute_code = "CATZOC", .int_value = 2}},
                      .instructions = {{.kind = RuleInstructionKind::kAddAreaOverlay, .palette_id = "CHGRD", .overlay_kind = AreaOverlayKind::kDataQualityA2, .overlay_placement = AreaOverlayPlacement::kTileInArea, .overlay_spacing_px = 86, .overlay_line_width_px = 2}},
                  },
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kAttributeIntContains, .attribute_code = "CATZOC", .int_value = 3}},
                      .instructions = {{.kind = RuleInstructionKind::kAddAreaOverlay, .palette_id = "CHGRD", .overlay_kind = AreaOverlayKind::kDataQualityB, .overlay_placement = AreaOverlayPlacement::kTileInArea, .overlay_spacing_px = 86, .overlay_line_width_px = 2}},
                  },
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kAttributeIntContains, .attribute_code = "CATZOC", .int_value = 4}},
                      .instructions = {{.kind = RuleInstructionKind::kAddAreaOverlay, .palette_id = "CHGRD", .overlay_kind = AreaOverlayKind::kDataQualityC, .overlay_placement = AreaOverlayPlacement::kTileInArea, .overlay_spacing_px = 86, .overlay_line_width_px = 2}},
                  },
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kAttributeIntContains, .attribute_code = "CATZOC", .int_value = 5}},
                      .instructions = {{.kind = RuleInstructionKind::kAddAreaOverlay, .palette_id = "CHGRD", .overlay_kind = AreaOverlayKind::kDataQualityD, .overlay_placement = AreaOverlayPlacement::kTileInArea, .overlay_spacing_px = 86, .overlay_line_width_px = 2}},
                  },
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kAttributeIntContains, .attribute_code = "CATZOC", .int_value = 6}},
                      .instructions = {{.kind = RuleInstructionKind::kAddAreaOverlay, .palette_id = "CHGRD", .overlay_kind = AreaOverlayKind::kDataQualityUnknown, .overlay_placement = AreaOverlayPlacement::kTileInArea, .overlay_spacing_px = 86, .overlay_line_width_px = 2}},
                  },
              },
          .fallback =
              {
                  RuleInstruction{
                      .kind = RuleInstructionKind::kAddAreaOverlay,
                      .palette_id = "CHGRD",
                      .overlay_kind = AreaOverlayKind::kNoData,
                      .overlay_placement = AreaOverlayPlacement::kTileInArea,
                      .overlay_spacing_px = 86,
                      .overlay_line_width_px = 2,
                  },
              },
      },
  };

  for (const auto& program : kPrograms) {
    if (program.object_class_acronym == object_class_acronym) {
      return &program;
    }
  }
  return nullptr;
}

bool ExecuteAreaProgram(const RuleProgram& program,
                        const AttributeList& attributes,
                        AreaExecutionState* state,
                        const DisplaySettings& settings,
                        const S57PortrayalCatalog& catalog) {
  if (state == nullptr) {
    return false;
  }

  ApplyInstructions(program.prefix, state, settings, catalog);

  bool matched = false;
  for (const auto& branch : program.branches) {
    if (!MatchesAllConditions(branch.conditions, attributes)) {
      continue;
    }
    ApplyInstructions(branch.instructions, state, settings, catalog);
    matched = true;
    if (branch.stop_after_match) {
      break;
    }
  }

  if (!matched) {
    ApplyInstructions(program.fallback, state, settings, catalog);
  }
  return true;
}

bool ApplyDepthAreaRuleProgram(const render::PolygonPrimitive& polygon,
                               FillStyle* fill,
                               StrokeStyle* stroke,
                               const DisplaySettings& settings,
                               const S57PortrayalCatalog& catalog) {
  if (fill == nullptr || stroke == nullptr) {
    return false;
  }

  if (polygon.object_class_acronym != "DEPARE" &&
      polygon.object_class_acronym != "DRGARE") {
    return false;
  }

  AreaExecutionState state{
      .fill = *fill,
      .stroke = *stroke,
      .overlays = {},
  };

  if (!HasAnyDepthValue(polygon.attributes)) {
    state.fill.enabled = true;
    state.stroke.enabled = true;
    state.stroke.pattern = StrokePatternKind::kSolid;
    state.stroke.width_px = 2;
    SetAreaFillPalette(&state, "NODTA", settings, catalog);
    SetAreaStrokePalette(&state, "CHGRD", settings, catalog);
  }

  if (polygon.object_class_acronym == "DRGARE") {
    state.stroke.enabled = true;
    state.stroke.pattern = StrokePatternKind::kDash;
    state.stroke.width_px = 1;
    SetAreaStrokePalette(&state, "CHGRF", settings, catalog);
  }

  *fill = std::move(state.fill);
  *stroke = std::move(state.stroke);
  return true;
}

std::vector<AreaOverlayStyle> ResolveDepthAreaRuleProgramOverlays(
    const render::PolygonPrimitive& polygon,
    const DisplaySettings& settings,
    const S57PortrayalCatalog& catalog,
    bool* handled) {
  if (polygon.object_class_acronym != "DEPARE" &&
      polygon.object_class_acronym != "DRGARE") {
    if (handled != nullptr) {
      *handled = false;
    }
    return {};
  }

  if (handled != nullptr) {
    *handled = true;
  }

  if (!HasAnyDepthValue(polygon.attributes)) {
    return {MakeAreaOverlayStyle(AreaOverlayKind::kSurveyReliability,
                                 AreaOverlayPlacement::kTileInArea,
                                 "CHGRD",
                                 92,
                                 2,
                                 settings,
                                 catalog)};
  }

  if (polygon.object_class_acronym == "DRGARE") {
    return {MakeAreaOverlayStyle(AreaOverlayKind::kDredgedArea,
                                 AreaOverlayPlacement::kTileInArea,
                                 "CHGRD",
                                 56,
                                 1,
                                 settings,
                                 catalog)};
  }

  return {};
}

}  // namespace

bool ApplyAreaRuleProgram(const render::PolygonPrimitive& polygon,
                          FillStyle* fill,
                          StrokeStyle* stroke,
                          const DisplaySettings& settings,
                          const S57PortrayalCatalog& catalog) {
  if (fill == nullptr || stroke == nullptr) {
    return false;
  }

  if (ApplyDepthAreaRuleProgram(polygon, fill, stroke, settings, catalog)) {
    return true;
  }

  const RuleProgram* program = FindAreaRuleProgram(polygon.object_class_acronym);
  if (program == nullptr) {
    return false;
  }

  AreaExecutionState state{
      .fill = *fill,
      .stroke = *stroke,
      .overlays = {},
  };
  ExecuteAreaProgram(*program, polygon.attributes, &state, settings, catalog);
  *fill = std::move(state.fill);
  *stroke = std::move(state.stroke);
  return true;
}

std::vector<AreaOverlayStyle> ResolveAreaRuleProgramOverlays(
    const render::PolygonPrimitive& polygon,
    const DisplaySettings& settings,
    const S57PortrayalCatalog& catalog,
    bool* handled) {
  std::vector<AreaOverlayStyle> depth_overlays =
      ResolveDepthAreaRuleProgramOverlays(polygon, settings, catalog, handled);
  if (handled != nullptr && *handled) {
    return depth_overlays;
  }

  const RuleProgram* program = FindAreaRuleProgram(polygon.object_class_acronym);
  if (program == nullptr) {
    if (handled != nullptr) {
      *handled = false;
    }
    return {};
  }

  if (handled != nullptr) {
    *handled = true;
  }

  AreaExecutionState state{};
  ExecuteAreaProgram(*program, polygon.attributes, &state, settings, catalog);
  return state.overlays;
}

}  // namespace navscene::portrayal::detail
