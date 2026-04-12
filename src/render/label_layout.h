#pragma once

#include "portrayal/scene.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace navscene::render {

struct ScreenPoint {
  double x = 0.0;
  double y = 0.0;
};

struct ScreenRect {
  double min_x = 0.0;
  double min_y = 0.0;
  double max_x = 0.0;
  double max_y = 0.0;
};

struct PlacedLabel {
  int priority = 0;
  std::string dataset_path;
  int64_t feature_id = -1;
  std::string object_class_acronym;
  std::string text;
  portrayal::TextStyle style;
  ScreenPoint anchor;
  ScreenPoint origin;
  ScreenRect bounds;
  bool important = false;
};

struct LabelCollisionBox {
  int priority = 0;
  std::string dataset_path;
  int64_t feature_id = -1;
  std::string object_class_acronym;
  ScreenRect bounds;
  bool important = false;
  bool accepted = false;
};

struct LabelLayoutResult {
  std::vector<PlacedLabel> placed_labels;
  std::vector<LabelCollisionBox> collision_boxes;
};

LabelLayoutResult LayoutPortrayalLabelsDetailed(
    const portrayal::PortrayalScene& scene,
    uint32_t width,
    uint32_t height,
    const std::function<ScreenPoint(const GeoPoint&)>& project);

std::vector<PlacedLabel> LayoutPortrayalLabels(
    const portrayal::PortrayalScene& scene,
    uint32_t width,
    uint32_t height,
    const std::function<ScreenPoint(const GeoPoint&)>& project);

}  // namespace navscene::render
