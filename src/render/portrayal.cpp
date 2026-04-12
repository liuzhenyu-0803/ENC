#include "render/portrayal.h"

namespace navscene::render {
namespace {

portrayal::DisplaySettings DefaultSettings() {
  auto settings = portrayal::MakeDisplaySettings(DisplayOptions{});
  settings.display_category = DisplayCategory::kAll;
  settings.show_meta = true;
  settings.show_quality_of_data = true;
  return settings;
}

}  // namespace

AreaPaintStyle ResolveAreaPaintStyle(const PolygonPrimitive& polygon) {
  const auto& catalog = portrayal::DefaultS57PortrayalCatalog();
  const auto lookup = catalog.ResolveLookup(portrayal::FeaturePortrayalContext{
      .lookup_key =
          portrayal::LookupKey{
              .geometry_kind = portrayal::GeometryKind::kArea,
              .object_class_acronym = polygon.object_class_acronym,
              .display_category = DefaultSettings().display_category,
          },
      .attributes = &polygon.attributes,
      .settings = nullptr,
  });
  const auto* style =
      catalog.FindAreaStyle(lookup.rule != nullptr ? lookup.style_id : "default_area");
  if (style == nullptr) {
    return {};
  }

  return AreaPaintStyle{
      .fill = style->fill.color,
      .stroke = style->stroke.color,
      .visible = style->fill.enabled || style->stroke.enabled,
  };
}

LinePaintStyle ResolveLinePaintStyle(const PolylinePrimitive& polyline) {
  const auto& catalog = portrayal::DefaultS57PortrayalCatalog();
  const auto lookup = catalog.ResolveLookup(portrayal::FeaturePortrayalContext{
      .lookup_key =
          portrayal::LookupKey{
              .geometry_kind = portrayal::GeometryKind::kLine,
              .object_class_acronym = polyline.object_class_acronym,
              .display_category = DefaultSettings().display_category,
          },
      .attributes = &polyline.attributes,
      .settings = nullptr,
  });
  const auto* style =
      catalog.FindLineStyle(lookup.rule != nullptr ? lookup.style_id : "default_line");
  if (style == nullptr) {
    return {};
  }

  return LinePaintStyle{
      .stroke = style->stroke.color,
      .width = style->stroke.width_px,
      .pattern = style->stroke.pattern,
      .visible = style->stroke.enabled,
  };
}

PointPaintStyle ResolvePointPaintStyle(const PointPrimitive& point) {
  const auto& catalog = portrayal::DefaultS57PortrayalCatalog();
  const auto lookup = catalog.ResolveLookup(portrayal::FeaturePortrayalContext{
      .lookup_key =
          portrayal::LookupKey{
              .geometry_kind = portrayal::GeometryKind::kPoint,
              .object_class_acronym = point.object_class_acronym,
              .display_category = DefaultSettings().display_category,
          },
      .attributes = &point.attributes,
      .settings = nullptr,
  });
  const auto* style =
      catalog.FindPointStyle(lookup.rule != nullptr ? lookup.style_id : "default_point");
  if (style == nullptr) {
    return {};
  }

  return PointPaintStyle{
      .kind = style->symbol.kind,
      .fill = style->symbol.fill,
      .stroke = style->symbol.stroke,
      .radius = style->symbol.size_px / 2,
      .visible = style->symbol.enabled,
  };
}

}  // namespace navscene::render
