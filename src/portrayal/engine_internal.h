#pragma once

#include "portrayal/engine.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace navscene::portrayal::detail {

Rgb8 MixColors(const Rgb8& foreground, const Rgb8& background, double foreground_weight);
Rgb8 ResolveBackgroundColor(const DisplaySettings& settings,
                            const S57PortrayalCatalog& catalog);
Rgb8 ResolvePaletteColor(const S57PortrayalCatalog& catalog,
                         const DisplaySettings& settings,
                         std::string_view palette_color_id,
                         const Rgb8& fallback);

FillStyle ResolveFillStyle(const FillStyle& style,
                           const DisplaySettings& settings,
                           const S57PortrayalCatalog& catalog);
AreaOverlayStyle ResolveAreaOverlayStyle(const AreaOverlayStyle& style,
                                         const DisplaySettings& settings,
                                         const S57PortrayalCatalog& catalog);
StrokeStyle ResolveStrokeStyle(const StrokeStyle& style,
                               const DisplaySettings& settings,
                               const S57PortrayalCatalog& catalog);
PointSymbolStyle ResolvePointSymbolStyle(const PointSymbolStyle& style,
                                         const DisplaySettings& settings,
                                         const S57PortrayalCatalog& catalog);
TextStyle ResolveTextStyle(const TextStyle& style,
                           const DisplaySettings& settings,
                           const S57PortrayalCatalog& catalog);

template <typename Command>
void SortByPriority(std::vector<Command>* commands) {
  if (commands == nullptr) {
    return;
  }

  std::sort(commands->begin(),
            commands->end(),
            [](const Command& lhs, const Command& rhs) {
              if (lhs.priority != rhs.priority) {
                return lhs.priority < rhs.priority;
              }
              if (lhs.geometry.object_class_acronym != rhs.geometry.object_class_acronym) {
                return lhs.geometry.object_class_acronym < rhs.geometry.object_class_acronym;
              }
              if (lhs.geometry.dataset_path != rhs.geometry.dataset_path) {
                return lhs.geometry.dataset_path < rhs.geometry.dataset_path;
              }
              return lhs.geometry.feature_id < rhs.geometry.feature_id;
            });
}

void SortLabels(std::vector<LabelCandidate>* labels);
void PopulateSceneStats(PortrayalScene* scene);

std::string_view FindAttributeValue(const AttributeList& attributes, std::string_view code);
std::optional<double> ParseDoubleAttribute(const AttributeList& attributes,
                                           std::string_view code);

bool IsMetaObjectClass(std::string_view object_class_acronym);
bool IsQualityOfDataObject(std::string_view object_class_acronym);
bool HasEnabledAreaOverlay(const std::vector<AreaOverlayStyle>& overlays);
bool IsLightObjectClass(std::string_view object_class_acronym);
bool IsStandaloneLightObjectClass(std::string_view object_class_acronym);
bool IsNavigationMarkObjectClass(std::string_view object_class_acronym);
bool IsBeaconObjectClass(std::string_view object_class_acronym);
bool IsBuoyObjectClass(std::string_view object_class_acronym);

std::vector<std::string> SplitAttributeValue(std::string_view value);
std::optional<int> ParseIntAttributeValue(std::string_view value);
bool AttributeContainsIntValue(const AttributeList& attributes,
                               std::string_view code,
                               int expected_value);
std::optional<int> FindFirstAttributeIntValue(const AttributeList& attributes,
                                              std::string_view code);
bool HasAnyDepthValue(const AttributeList& attributes);
bool HasLowAccuracyPositioning(const AttributeList& attributes);

AreaOverlayStyle MakeAreaOverlayStyle(AreaOverlayKind kind,
                                      AreaOverlayPlacement placement,
                                      std::string_view palette_id,
                                      int spacing_px,
                                      int line_width_px,
                                      const DisplaySettings& settings,
                                      const S57PortrayalCatalog& catalog);

bool IsRestrictionAreaObjectClass(std::string_view object_class_acronym);
bool PassesCommonFilters(std::string_view object_class_acronym,
                         const AttributeList& attributes,
                         const DisplaySettings& settings);

std::string_view ResolveDerivedAreaStyleId(const render::PolygonPrimitive& polygon,
                                           const DisplaySettings& settings,
                                           std::string_view fallback_style_id);
std::string_view ResolveDerivedLineStyleId(const render::PolylinePrimitive& polyline,
                                           const DisplaySettings& settings,
                                           std::string_view fallback_style_id);
std::string_view ResolveDerivedPointStyleId(const render::PointPrimitive& point,
                                            const DisplaySettings& settings,
                                            std::string_view fallback_style_id);

void ResolveProceduralAreaStyle(const render::PolygonPrimitive& polygon,
                                FillStyle* fill,
                                StrokeStyle* stroke,
                                const DisplaySettings& settings,
                                const S57PortrayalCatalog& catalog);
std::vector<AreaOverlayStyle> ResolveProceduralAreaOverlays(
    const render::PolygonPrimitive& polygon,
    const DisplaySettings& settings,
    const S57PortrayalCatalog& catalog);

void ResolveProceduralLineStyle(const render::PolylinePrimitive& polyline,
                                StrokeStyle* stroke,
                                const DisplaySettings& settings,
                                const S57PortrayalCatalog& catalog);

PointSymbolStyle ResolveProceduralPointStyle(const render::PointPrimitive& point,
                                             PointSymbolStyle style,
                                             const DisplaySettings& settings,
                                             const S57PortrayalCatalog& catalog);

std::vector<PointCommand> ResolveProceduralPointDecorations(
    const render::PointPrimitive& point,
    int base_priority,
    const DisplaySettings& settings,
    const S57PortrayalCatalog& catalog);

GeoPoint ComputePolylineAnchor(const render::PolylinePrimitive& polyline);
GeoPoint ComputePolygonAnchor(const render::PolygonPrimitive& polygon);
std::optional<LabelCandidate> BuildLabelCandidate(int priority,
                                                  const std::string& dataset_path,
                                                  int64_t feature_id,
                                                  const std::string& object_class_acronym,
                                                  const AttributeList& attributes,
                                                  const GeoPoint& anchor,
                                                  const LookupResult& lookup,
                                                  const DisplaySettings& settings,
                                                  const S57PortrayalCatalog& catalog);

}  // namespace navscene::portrayal::detail
