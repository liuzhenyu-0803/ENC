#pragma once

#include "portrayal/engine_internal.h"

#include <string>
#include <string_view>
#include <vector>

namespace navscene::portrayal::detail {

enum class RuleConditionKind {
  kAttributeIntContains = 0,
  kAttributeIntFirstEquals,
  kHasAnyDepthValue,
  kHasLowAccuracyPositioning,
};

struct RuleCondition {
  RuleConditionKind kind = RuleConditionKind::kAttributeIntContains;
  std::string attribute_code;
  int int_value = 0;
};

enum class RuleInstructionKind {
  kSetFillEnabled = 0,
  kSetStrokeEnabled,
  kSetFillPalette,
  kSetStrokePalette,
  kSetStrokePattern,
  kSetStrokeWidth,
  kMixFillWithBackground,
  kClearFillPalette,
  kAddAreaOverlay,
};

struct RuleInstruction {
  RuleInstructionKind kind = RuleInstructionKind::kSetFillEnabled;
  bool bool_value = false;
  int int_value = 0;
  double scalar_value = 0.0;
  std::string palette_id;
  StrokePatternKind stroke_pattern = StrokePatternKind::kSolid;
  AreaOverlayKind overlay_kind = AreaOverlayKind::kNone;
  AreaOverlayPlacement overlay_placement = AreaOverlayPlacement::kTileInArea;
  int overlay_spacing_px = 64;
  int overlay_line_width_px = 1;
};

struct RuleBranch {
  std::vector<RuleCondition> conditions;
  std::vector<RuleInstruction> instructions;
  bool stop_after_match = true;
};

struct RuleProgram {
  GeometryKind geometry_kind = GeometryKind::kArea;
  std::string_view object_class_acronym;
  std::vector<RuleInstruction> prefix;
  std::vector<RuleBranch> branches;
  std::vector<RuleInstruction> fallback;
};

bool ApplyAreaRuleProgram(const render::PolygonPrimitive& polygon,
                          FillStyle* fill,
                          StrokeStyle* stroke,
                          const DisplaySettings& settings,
                          const S57PortrayalCatalog& catalog);

std::vector<AreaOverlayStyle> ResolveAreaRuleProgramOverlays(
    const render::PolygonPrimitive& polygon,
    const DisplaySettings& settings,
    const S57PortrayalCatalog& catalog,
    bool* handled);

bool ApplyLineRuleProgram(const render::PolylinePrimitive& polyline,
                          StrokeStyle* stroke,
                          const DisplaySettings& settings,
                          const S57PortrayalCatalog& catalog);

bool ApplyPointRuleProgram(const render::PointPrimitive& point,
                           PointSymbolStyle* style,
                           const DisplaySettings& settings,
                           const S57PortrayalCatalog& catalog);

}  // namespace navscene::portrayal::detail
