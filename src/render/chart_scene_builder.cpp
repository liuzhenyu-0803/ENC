#include "render/chart_scene_builder.h"

namespace navscene::render {

void AppendDatasetToChartScene(const data::s57::DatasetInfo& dataset,
                               const SceneBuildOptions&,
                               ChartScene* out) {
  if (out == nullptr) {
    return;
  }

  const std::string& dataset_path = dataset.descriptor.path;

  for (const auto& feature : dataset.geometry.point_features) {
    for (const auto& point : feature.points) {
      out->points.push_back(PointPrimitive{
          .dataset_path = dataset_path,
          .feature_id = feature.feature_id,
          .source_layer = feature.source_layer,
          .object_class_code = feature.object_class_code,
          .object_class_name = feature.object_class_name,
          .object_class_acronym = feature.object_class_acronym,
          .attributes = feature.attributes,
          .position = point.position,
      });
    }
  }

  for (const auto& feature : dataset.geometry.line_features) {
    for (const auto& part : feature.parts) {
      if (part.vertices.empty()) {
        continue;
      }

      out->polylines.push_back(PolylinePrimitive{
          .dataset_path = dataset_path,
          .feature_id = feature.feature_id,
          .source_layer = feature.source_layer,
          .object_class_code = feature.object_class_code,
          .object_class_name = feature.object_class_name,
          .object_class_acronym = feature.object_class_acronym,
          .attributes = feature.attributes,
          .vertices = part.vertices,
      });
    }
  }

  for (const auto& feature : dataset.geometry.area_features) {
    for (const auto& polygon : feature.polygons) {
      if (polygon.outer_ring.empty()) {
        continue;
      }

      out->polygons.push_back(PolygonPrimitive{
          .dataset_path = dataset_path,
          .feature_id = feature.feature_id,
          .source_layer = feature.source_layer,
          .object_class_code = feature.object_class_code,
          .object_class_name = feature.object_class_name,
          .object_class_acronym = feature.object_class_acronym,
          .attributes = feature.attributes,
          .outer_ring = polygon.outer_ring,
          .holes = polygon.holes,
      });
    }
  }

  out->stats.point_primitive_count = static_cast<uint64_t>(out->points.size());
  out->stats.polyline_primitive_count = static_cast<uint64_t>(out->polylines.size());
  out->stats.polygon_primitive_count = static_cast<uint64_t>(out->polygons.size());
}

void AppendDatasetToChartScene(const data::s57::DatasetInfo& dataset, ChartScene* out) {
  AppendDatasetToChartScene(dataset, SceneBuildOptions{}, out);
}

}  // namespace navscene::render
