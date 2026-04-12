#pragma once

#include "portrayal/profile.h"

#include <string>
#include <string_view>
#include <vector>

namespace navscene::portrayal {

enum class GeometryKind {
  kPoint = 0,
  kLine,
  kArea,
};

struct AreaStyle {
  FillStyle fill;
  StrokeStyle stroke;
};

struct LineStyle {
  StrokeStyle stroke;
};

struct PointStyle {
  PointSymbolStyle symbol;
};

struct TextRoleStyle {
  TextStyle text;
};

struct AttributeCondition {
  std::string key;
  std::string value;
};

struct LookupRule {
  GeometryKind geometry_kind = GeometryKind::kArea;
  std::string object_class_acronym;
  DisplayCategory min_display_category = DisplayCategory::kBase;
  std::string style_id;
  std::string text_role_id;
  int priority = 0;
  bool generate_label = false;
  bool important_label = false;
  std::vector<AttributeCondition> attribute_conditions;
};

struct LookupKey {
  GeometryKind geometry_kind = GeometryKind::kArea;
  std::string_view object_class_acronym;
  DisplayCategory display_category = DisplayCategory::kBase;
};

struct FeaturePortrayalContext {
  LookupKey lookup_key;
  const AttributeList* attributes = nullptr;
  const DisplaySettings* settings = nullptr;
};

struct LookupResult {
  const LookupRule* rule = nullptr;
  std::string_view style_id;
  const TextRoleStyle* text_role = nullptr;
};

class S57PortrayalCatalog {
 public:
  struct NamedColor {
    std::string id;
    Rgb8 day;
    Rgb8 dusk;
    Rgb8 night;
  };

  struct NamedAreaStyle {
    std::string id;
    AreaStyle style;
  };

  struct NamedLineStyle {
    std::string id;
    LineStyle style;
  };

  struct NamedPointStyle {
    std::string id;
    PointStyle style;
  };

  struct NamedTextRoleStyle {
    std::string id;
    TextRoleStyle style;
  };

  PortrayalProfileId profile_id() const { return profile_id_; }
  void set_profile_id(PortrayalProfileId value) { profile_id_ = value; }

  const std::vector<NamedColor>& colors() const { return colors_; }
  const std::vector<NamedAreaStyle>& area_styles() const { return area_styles_; }
  const std::vector<NamedLineStyle>& line_styles() const { return line_styles_; }
  const std::vector<NamedPointStyle>& point_styles() const { return point_styles_; }
  const std::vector<NamedTextRoleStyle>& text_roles() const { return text_roles_; }
  const std::vector<LookupRule>& lookup_rules() const { return lookup_rules_; }

  std::vector<NamedColor>& mutable_colors() { return colors_; }
  std::vector<NamedAreaStyle>& mutable_area_styles() { return area_styles_; }
  std::vector<NamedLineStyle>& mutable_line_styles() { return line_styles_; }
  std::vector<NamedPointStyle>& mutable_point_styles() { return point_styles_; }
  std::vector<NamedTextRoleStyle>& mutable_text_roles() { return text_roles_; }
  std::vector<LookupRule>& mutable_lookup_rules() { return lookup_rules_; }

  LookupResult ResolveLookup(const FeaturePortrayalContext& context) const;
  const LookupRule* ResolveRule(GeometryKind geometry_kind,
                                std::string_view object_class_acronym,
                                const DisplaySettings& settings) const;

  const AreaStyle* FindAreaStyle(std::string_view style_id) const;
  const LineStyle* FindLineStyle(std::string_view style_id) const;
  const PointStyle* FindPointStyle(std::string_view style_id) const;
  const TextRoleStyle* FindTextRole(std::string_view style_id) const;
  const Rgb8* FindColor(std::string_view color_id,
                        ColorScheme scheme = ColorScheme::kDay) const;

  Status Validate(std::string* details = nullptr) const;

 private:
  PortrayalProfileId profile_id_ = PortrayalProfileId::kS57;
  std::vector<NamedColor> colors_;
  std::vector<NamedAreaStyle> area_styles_;
  std::vector<NamedLineStyle> line_styles_;
  std::vector<NamedPointStyle> point_styles_;
  std::vector<NamedTextRoleStyle> text_roles_;
  std::vector<LookupRule> lookup_rules_;
};

Status LoadS57PortrayalCatalogFromFile(const std::string& path,
                                       S57PortrayalCatalog* out);

const S57PortrayalCatalog& DefaultS57PortrayalCatalog();

}  // namespace navscene::portrayal
