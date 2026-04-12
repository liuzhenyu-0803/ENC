#pragma once

#include "navscene/navscene.h"

#include <cstdint>
#include <string>
#include <vector>

namespace navscene::data::s57 {

struct PointGeometry {
  GeoPoint position;
};

struct LineStringGeometry {
  std::vector<GeoPoint> vertices;
};

struct PolygonGeometry {
  std::vector<GeoPoint> outer_ring;
  std::vector<std::vector<GeoPoint>> holes;
};

struct PointFeatureGeometry {
  int64_t feature_id = -1;
  std::string source_layer;
  int object_class_code = 0;
  std::string object_class_name;
  std::string object_class_acronym;
  AttributeList attributes;
  std::vector<PointGeometry> points;
};

struct LineFeatureGeometry {
  int64_t feature_id = -1;
  std::string source_layer;
  int object_class_code = 0;
  std::string object_class_name;
  std::string object_class_acronym;
  AttributeList attributes;
  std::vector<LineStringGeometry> parts;
};

struct AreaFeatureGeometry {
  int64_t feature_id = -1;
  std::string source_layer;
  int object_class_code = 0;
  std::string object_class_name;
  std::string object_class_acronym;
  AttributeList attributes;
  std::vector<PolygonGeometry> polygons;
};

struct GeometrySummary {
  uint64_t point_feature_count = 0;
  uint64_t line_feature_count = 0;
  uint64_t area_feature_count = 0;
  uint64_t point_instance_count = 0;
  uint64_t line_part_count = 0;
  uint64_t area_polygon_count = 0;
  uint64_t vertex_count = 0;
};

struct GeometryStore {
  std::vector<PointFeatureGeometry> point_features;
  std::vector<LineFeatureGeometry> line_features;
  std::vector<AreaFeatureGeometry> area_features;
  GeometrySummary summary;
};

}  // namespace navscene::data::s57
