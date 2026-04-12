#include "render/label_layout.h"

#include <algorithm>
#include <iostream>
#include <string_view>

namespace {

bool Expect(bool condition, std::string_view message) {
  if (condition) {
    return true;
  }

  std::cerr << "[navscene-label-layout] " << message << '\n';
  return false;
}

}  // namespace

int main() {
  navscene::portrayal::PortrayalScene scene;
  scene.labels.push_back(navscene::portrayal::LabelCandidate{
      .priority = 10,
      .dataset_path = "sample",
      .feature_id = 1,
      .object_class_acronym = "BOYLAT",
      .text = "Buoy",
      .anchor = {.lat = 0.0, .lon = 0.0},
      .style = {.role = navscene::portrayal::FontRole::kBody,
                .color = {10, 20, 30},
                .size_px = 12,
                .halo = false},
      .important = false,
  });
  scene.labels.push_back(navscene::portrayal::LabelCandidate{
      .priority = 50,
      .dataset_path = "sample",
      .feature_id = 2,
      .object_class_acronym = "SOUNDG",
      .text = "12.4",
      .anchor = {.lat = 0.0, .lon = 0.0},
      .style = {.role = navscene::portrayal::FontRole::kSounding,
                .color = {40, 50, 60},
                .size_px = 11,
                .halo = true},
      .important = true,
  });
  scene.labels.push_back(navscene::portrayal::LabelCandidate{
      .priority = 20,
      .dataset_path = "sample",
      .feature_id = 3,
      .object_class_acronym = "LIGHTS",
      .text = "Light",
      .anchor = {.lat = 1.0, .lon = 1.0},
      .style = {.role = navscene::portrayal::FontRole::kImportant,
                .color = {70, 80, 90},
                .size_px = 12,
                .halo = true},
      .important = true,
  });

  const auto layout = navscene::render::LayoutPortrayalLabelsDetailed(
      scene,
      200,
      160,
      [](const navscene::GeoPoint& point) {
        return navscene::render::ScreenPoint{
            .x = 50.0 + point.lon * 20.0,
            .y = 80.0 - point.lat * 20.0,
        };
      });
  const auto& placed = layout.placed_labels;

  if (!Expect(placed.size() == 2,
              "Overlapping labels should be decluttered while keeping separate labels.")) {
    return 1;
  }
  if (!Expect(placed.front().text == "12.4",
              "Important higher-priority labels should survive collisions first.")) {
    return 1;
  }
  if (!Expect(placed.back().text == "Light",
              "Non-overlapping important labels should also be kept.")) {
    return 1;
  }
  if (!Expect(placed.front().bounds.max_x > placed.front().bounds.min_x &&
                  placed.front().bounds.max_y > placed.front().bounds.min_y,
              "Placed labels should expose valid screen-space bounds.")) {
    return 1;
  }
  if (!Expect(layout.collision_boxes.size() == 3,
              "Detailed label layout should expose every evaluated collision box.")) {
    return 1;
  }
  const auto rejected_count =
      static_cast<size_t>(std::count_if(layout.collision_boxes.begin(),
                                        layout.collision_boxes.end(),
                                        [](const navscene::render::LabelCollisionBox& box) {
                                          return !box.accepted;
                                        }));
  if (!Expect(rejected_count == 1,
              "Exactly one label should be rejected by the collision pass in this scenario.")) {
    return 1;
  }

  return 0;
}
