#include "portrayal/rule_program_internal.h"

#include <cmath>

namespace navscene::portrayal::detail {
namespace {

bool IsCloseTo(double lhs, double rhs) {
  return std::abs(lhs - rhs) <= 1e-3;
}

void ApplyLineInstructions(const std::vector<RuleInstruction>& instructions,
                           StrokeStyle* stroke,
                           const DisplaySettings& settings,
                           const S57PortrayalCatalog& catalog) {
  if (stroke == nullptr) {
    return;
  }

  for (const auto& instruction : instructions) {
    switch (instruction.kind) {
      case RuleInstructionKind::kSetStrokeEnabled:
        stroke->enabled = instruction.bool_value;
        break;
      case RuleInstructionKind::kSetStrokePalette:
        stroke->palette_color_id = instruction.palette_id;
        stroke->color = ResolvePaletteColor(
            catalog, settings, instruction.palette_id, stroke->color);
        break;
      case RuleInstructionKind::kSetStrokePattern:
        stroke->pattern = instruction.stroke_pattern;
        break;
      case RuleInstructionKind::kSetStrokeWidth:
        stroke->width_px = instruction.int_value;
        break;
      default:
        break;
    }
  }
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

bool ExecuteLineProgram(const RuleProgram& program,
                        const AttributeList& attributes,
                        StrokeStyle* stroke,
                        const DisplaySettings& settings,
                        const S57PortrayalCatalog& catalog) {
  ApplyLineInstructions(program.prefix, stroke, settings, catalog);

  bool matched = false;
  for (const auto& branch : program.branches) {
    if (!MatchesAllConditions(branch.conditions, attributes)) {
      continue;
    }
    ApplyLineInstructions(branch.instructions, stroke, settings, catalog);
    matched = true;
    if (branch.stop_after_match) {
      break;
    }
  }

  if (!matched) {
    ApplyLineInstructions(program.fallback, stroke, settings, catalog);
  }
  return true;
}

const RuleProgram* FindLineRuleProgram(std::string_view object_class_acronym) {
  static const std::vector<RuleProgram> kPrograms = {
      RuleProgram{
          .geometry_kind = GeometryKind::kLine,
          .object_class_acronym = "COALNE",
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
              },
      },
      RuleProgram{
          .geometry_kind = GeometryKind::kLine,
          .object_class_acronym = "SLCONS",
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
              },
      },
      RuleProgram{
          .geometry_kind = GeometryKind::kLine,
          .object_class_acronym = "BRIDGE",
          .prefix =
              {
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kSolid},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 4},
                  RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CHGRD"},
              },
      },
      RuleProgram{
          .geometry_kind = GeometryKind::kLine,
          .object_class_acronym = "OBSTRN",
          .branches =
              {
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kAttributeIntContains, .attribute_code = "CATOBS", .int_value = 9}},
                      .instructions =
                          {
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kDash},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 1},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CHMGD"},
                          },
                  },
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kAttributeIntContains, .attribute_code = "CATOBS", .int_value = 7}},
                      .instructions =
                          {
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kDash},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 1},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CHGRD"},
                          },
                  },
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kAttributeIntContains, .attribute_code = "CATOBS", .int_value = 8}},
                      .instructions =
                          {
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kDash},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 1},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CSTLN"},
                          },
                  },
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kAttributeIntContains, .attribute_code = "CATOBS", .int_value = 10}},
                      .instructions =
                          {
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kDash},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 1},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CSTLN"},
                          },
                  },
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kAttributeIntContains, .attribute_code = "WATLEV", .int_value = 7}},
                      .instructions =
                          {
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kDash},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 1},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CSTLN"},
                          },
                  },
                  RuleBranch{
                      .conditions = {{.kind = RuleConditionKind::kAttributeIntContains, .attribute_code = "CATOBS", .int_value = 6}},
                      .instructions =
                          {
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePattern, .stroke_pattern = StrokePatternKind::kDot},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokeWidth, .int_value = 2},
                              RuleInstruction{.kind = RuleInstructionKind::kSetStrokePalette, .palette_id = "CHBLK"},
                          },
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

bool ApplyDepthContourRuleProgram(const render::PolylinePrimitive& polyline,
                                  StrokeStyle* stroke,
                                  const DisplaySettings& settings,
                                  const S57PortrayalCatalog& catalog) {
  if (stroke == nullptr || polyline.object_class_acronym != "DEPCNT") {
    return false;
  }

  const auto contour = ParseDoubleAttribute(polyline.attributes, "VALDCO");
  const bool is_safety_contour =
      contour.has_value() && IsCloseTo(*contour, settings.safety_contour_m);

  stroke->pattern =
      HasLowAccuracyPositioning(polyline.attributes) ? StrokePatternKind::kDash
                                                     : StrokePatternKind::kSolid;
  stroke->width_px = is_safety_contour ? 2 : 1;
  stroke->palette_color_id = is_safety_contour ? "DEPSC" : "DEPCN";
  stroke->color = ResolvePaletteColor(
      catalog, settings, stroke->palette_color_id, stroke->color);
  return true;
}

}  // namespace

bool ApplyLineRuleProgram(const render::PolylinePrimitive& polyline,
                          StrokeStyle* stroke,
                          const DisplaySettings& settings,
                          const S57PortrayalCatalog& catalog) {
  if (stroke == nullptr) {
    return false;
  }

  if (ApplyDepthContourRuleProgram(polyline, stroke, settings, catalog)) {
    return true;
  }

  const RuleProgram* program = FindLineRuleProgram(polyline.object_class_acronym);
  if (program == nullptr) {
    return false;
  }

  ExecuteLineProgram(*program, polyline.attributes, stroke, settings, catalog);
  return true;
}

}  // namespace navscene::portrayal::detail
