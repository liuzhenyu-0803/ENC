#pragma once

#include "data/s57/geometry.h"
#include "navscene/navscene.h"

#include <cstdint>
#include <string>
#include <vector>

namespace navscene::data::s57 {

struct FeatureLayerInfo {
  std::string layer_name;
  uint64_t feature_count = 0;
  bool has_geometry = false;
};

struct DatasetInfo {
  DatasetDescriptor descriptor;
  std::string dataset_name;
  std::string edition;
  std::string update_number;
  std::string issue_date;
  std::string reader_name;
  uint64_t feature_count = 0;
  bool metadata_complete = false;
  bool geometry_loaded = false;
  std::vector<FeatureLayerInfo> layers;
  GeometryStore geometry;
};

}  // namespace navscene::data::s57
