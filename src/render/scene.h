#pragma once

#include "navscene/navscene.h"

#include <cstdint>
#include <string>
#include <vector>

namespace navscene::render {

struct PointPrimitive {
  std::string dataset_path;
  int64_t feature_id = -1;
  std::string source_layer;
  int object_class_code = 0;
  std::string object_class_name;
  std::string object_class_acronym;
  AttributeList attributes;
  GeoPoint position;
};

struct PolylinePrimitive {
  std::string dataset_path;
  int64_t feature_id = -1;
  std::string source_layer;
  int object_class_code = 0;
  std::string object_class_name;
  std::string object_class_acronym;
  AttributeList attributes;
  std::vector<GeoPoint> vertices;
};

struct PolygonPrimitive {
  std::string dataset_path;
  int64_t feature_id = -1;
  std::string source_layer;
  int object_class_code = 0;
  std::string object_class_name;
  std::string object_class_acronym;
  AttributeList attributes;
  std::vector<GeoPoint> outer_ring;
  std::vector<std::vector<GeoPoint>> holes;
};

struct ChartSceneStats {
  uint64_t point_primitive_count = 0;
  uint64_t polyline_primitive_count = 0;
  uint64_t polygon_primitive_count = 0;
};

struct ChartScene {
  std::vector<PointPrimitive> points;
  std::vector<PolylinePrimitive> polylines;
  std::vector<PolygonPrimitive> polygons;
  ChartSceneStats stats;
};

}  // namespace navscene::render
