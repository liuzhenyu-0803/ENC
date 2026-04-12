#include "portrayal/engine_internal.h"

namespace navscene::portrayal {

PortrayalScene BuildPortrayalScene(const render::ChartScene& scene,
                                   const DisplaySettings& settings,
                                   const S57PortrayalCatalog& catalog) {
  PortrayalScene portrayal_scene;
  portrayal_scene.display_settings = settings;
  portrayal_scene.background_color = detail::ResolveBackgroundColor(settings, catalog);

  for (const auto& polygon : scene.polygons) {
    if (!detail::PassesCommonFilters(polygon.object_class_acronym, polygon.attributes, settings)) {
      continue;
    }

    const LookupResult lookup = catalog.ResolveLookup(FeaturePortrayalContext{
        .lookup_key =
            LookupKey{
                .geometry_kind = GeometryKind::kArea,
                .object_class_acronym = polygon.object_class_acronym,
                .display_category = settings.display_category,
            },
        .attributes = &polygon.attributes,
        .settings = &settings,
    });
    if (lookup.rule == nullptr) {
      continue;
    }

    const std::string_view style_id =
        detail::ResolveDerivedAreaStyleId(polygon, settings, lookup.style_id);
    const AreaStyle* style = catalog.FindAreaStyle(style_id);
    if (style == nullptr) {
      continue;
    }

    FillStyle fill = detail::ResolveFillStyle(style->fill, settings, catalog);
    StrokeStyle stroke = detail::ResolveStrokeStyle(style->stroke, settings, catalog);
    detail::ResolveProceduralAreaStyle(polygon, &fill, &stroke, settings, catalog);
    std::vector<AreaOverlayStyle> overlays =
        detail::ResolveProceduralAreaOverlays(polygon, settings, catalog);

    if (fill.enabled || stroke.enabled || detail::HasEnabledAreaOverlay(overlays)) {
      portrayal_scene.areas.push_back(AreaCommand{
          .priority = lookup.rule->priority,
          .geometry = polygon,
          .fill = std::move(fill),
          .stroke = std::move(stroke),
          .overlays = std::move(overlays),
          .visible = true,
      });
    }

    if (const auto label =
            detail::BuildLabelCandidate(lookup.rule->priority,
                                        polygon.dataset_path,
                                        polygon.feature_id,
                                        polygon.object_class_acronym,
                                        polygon.attributes,
                                        detail::ComputePolygonAnchor(polygon),
                                        lookup,
                                        settings,
                                        catalog);
        label.has_value()) {
      portrayal_scene.labels.push_back(std::move(*label));
    }
  }

  for (const auto& polyline : scene.polylines) {
    if (!detail::PassesCommonFilters(polyline.object_class_acronym,
                                     polyline.attributes,
                                     settings)) {
      continue;
    }

    const LookupResult lookup = catalog.ResolveLookup(FeaturePortrayalContext{
        .lookup_key =
            LookupKey{
                .geometry_kind = GeometryKind::kLine,
                .object_class_acronym = polyline.object_class_acronym,
                .display_category = settings.display_category,
            },
        .attributes = &polyline.attributes,
        .settings = &settings,
    });
    if (lookup.rule == nullptr) {
      continue;
    }

    const std::string_view style_id =
        detail::ResolveDerivedLineStyleId(polyline, settings, lookup.style_id);
    const LineStyle* style = catalog.FindLineStyle(style_id);
    if (style == nullptr) {
      continue;
    }

    StrokeStyle stroke = detail::ResolveStrokeStyle(style->stroke, settings, catalog);
    detail::ResolveProceduralLineStyle(polyline, &stroke, settings, catalog);

    portrayal_scene.lines.push_back(LineCommand{
        .priority = lookup.rule->priority,
        .geometry = polyline,
        .stroke = std::move(stroke),
        .visible = true,
    });

    if (const auto label =
            detail::BuildLabelCandidate(lookup.rule->priority,
                                        polyline.dataset_path,
                                        polyline.feature_id,
                                        polyline.object_class_acronym,
                                        polyline.attributes,
                                        detail::ComputePolylineAnchor(polyline),
                                        lookup,
                                        settings,
                                        catalog);
        label.has_value()) {
      portrayal_scene.labels.push_back(std::move(*label));
    }
  }

  for (const auto& point : scene.points) {
    if (!detail::PassesCommonFilters(point.object_class_acronym, point.attributes, settings)) {
      continue;
    }

    const LookupResult lookup = catalog.ResolveLookup(FeaturePortrayalContext{
        .lookup_key =
            LookupKey{
                .geometry_kind = GeometryKind::kPoint,
                .object_class_acronym = point.object_class_acronym,
                .display_category = settings.display_category,
            },
        .attributes = &point.attributes,
        .settings = &settings,
    });
    if (lookup.rule == nullptr) {
      continue;
    }

    const std::string_view style_id =
        detail::ResolveDerivedPointStyleId(point, settings, lookup.style_id);
    const PointStyle* style = catalog.FindPointStyle(style_id);
    if (style == nullptr) {
      continue;
    }

    portrayal_scene.points.push_back(PointCommand{
        .priority = lookup.rule->priority,
        .geometry = point,
        .symbol = detail::ResolveProceduralPointStyle(
            point,
            detail::ResolvePointSymbolStyle(style->symbol, settings, catalog),
            settings,
            catalog),
        .visible = true,
    });

    std::vector<PointCommand> point_decorations =
        detail::ResolveProceduralPointDecorations(
            point, lookup.rule->priority, settings, catalog);
    portrayal_scene.points.insert(portrayal_scene.points.end(),
                                  point_decorations.begin(),
                                  point_decorations.end());

    if (const auto label =
            detail::BuildLabelCandidate(lookup.rule->priority,
                                        point.dataset_path,
                                        point.feature_id,
                                        point.object_class_acronym,
                                        point.attributes,
                                        point.position,
                                        lookup,
                                        settings,
                                        catalog);
        label.has_value()) {
      portrayal_scene.labels.push_back(std::move(*label));
    }
  }

  detail::SortByPriority(&portrayal_scene.areas);
  detail::SortByPriority(&portrayal_scene.lines);
  detail::SortByPriority(&portrayal_scene.points);
  detail::SortLabels(&portrayal_scene.labels);
  detail::PopulateSceneStats(&portrayal_scene);
  return portrayal_scene;
}

PortrayalScene BuildPortrayalScene(const render::ChartScene& scene,
                                   const DisplaySettings& settings,
                                   const IPortrayalProfile& profile) {
  return profile.BuildScene(scene, settings);
}

PortrayalScene BuildPortrayalScene(const render::ChartScene& scene,
                                   const DisplaySettings& settings,
                                   PortrayalProfileId profile_id) {
  if (const auto* profile = FindPortrayalProfile(profile_id)) {
    return profile->BuildScene(scene, settings);
  }
  return DefaultPortrayalProfile().BuildScene(scene, settings);
}

}  // namespace navscene::portrayal
