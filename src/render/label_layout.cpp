#include "render/label_layout.h"

#include <algorithm>

namespace navscene::render {
namespace {

constexpr double kAnchorOffsetX = 6.0;
constexpr double kAnchorOffsetY = -6.0;
constexpr double kLabelPaddingX = 4.0;
constexpr double kLabelPaddingY = 2.0;
constexpr double kApproxCharWidthFactor = 0.62;

bool IsCompactShortLabel(const portrayal::LabelCandidate& label) {
  return label.object_class_acronym == "SBDARE" && label.text.size() <= 4;
}

ScreenPoint ComputeLabelOrigin(const portrayal::LabelCandidate& label,
                               const ScreenPoint& anchor,
                               double text_width) {
  if (IsCompactShortLabel(label)) {
    return ScreenPoint{
        .x = anchor.x - text_width * 0.5,
        .y = anchor.y - 2.0,
    };
  }

  return ScreenPoint{
      .x = anchor.x + kAnchorOffsetX,
      .y = anchor.y + kAnchorOffsetY,
  };
}

bool Overlaps(const ScreenRect& lhs, const ScreenRect& rhs) {
  return lhs.min_x < rhs.max_x && lhs.max_x > rhs.min_x && lhs.min_y < rhs.max_y &&
         lhs.max_y > rhs.min_y;
}

bool IsOnScreen(const ScreenRect& bounds, uint32_t width, uint32_t height) {
  return bounds.max_x >= 0.0 && bounds.max_y >= 0.0 &&
         bounds.min_x <= static_cast<double>(width) &&
         bounds.min_y <= static_cast<double>(height);
}

ScreenRect EstimateBounds(const portrayal::LabelCandidate& label,
                          const ScreenPoint& anchor) {
  const double text_height = static_cast<double>(std::max(label.style.size_px, 8));
  const double text_width =
      static_cast<double>(label.text.size()) * text_height * kApproxCharWidthFactor;
  const bool is_compact_short_label = IsCompactShortLabel(label);
  const double padding_x = is_compact_short_label ? 1.0 : kLabelPaddingX;
  const double padding_y = is_compact_short_label ? 1.0 : kLabelPaddingY;
  const ScreenPoint origin = ComputeLabelOrigin(label, anchor, text_width);
  return ScreenRect{
      .min_x = origin.x - padding_x,
      .min_y = origin.y - text_height - padding_y,
      .max_x = origin.x + text_width + padding_x,
      .max_y = origin.y + padding_y,
  };
}

}  // namespace

LabelLayoutResult LayoutPortrayalLabelsDetailed(
    const portrayal::PortrayalScene& scene,
    uint32_t width,
    uint32_t height,
    const std::function<ScreenPoint(const GeoPoint&)>& project) {
  LabelLayoutResult result;
  std::vector<const portrayal::LabelCandidate*> ordered;
  ordered.reserve(scene.labels.size());
  for (const auto& label : scene.labels) {
    if (!label.text.empty()) {
      ordered.push_back(&label);
    }
  }

  std::sort(ordered.begin(),
            ordered.end(),
            [](const portrayal::LabelCandidate* lhs, const portrayal::LabelCandidate* rhs) {
              if (lhs->important != rhs->important) {
                return lhs->important && !rhs->important;
              }
              if (lhs->priority != rhs->priority) {
                return lhs->priority > rhs->priority;
              }
              if (lhs->object_class_acronym != rhs->object_class_acronym) {
                return lhs->object_class_acronym < rhs->object_class_acronym;
              }
              if (lhs->dataset_path != rhs->dataset_path) {
                return lhs->dataset_path < rhs->dataset_path;
              }
              return lhs->feature_id < rhs->feature_id;
            });

  result.placed_labels.reserve(ordered.size());
  result.collision_boxes.reserve(ordered.size());
  std::vector<ScreenRect> accepted_bounds;
  accepted_bounds.reserve(ordered.size());

  for (const auto* label : ordered) {
    if (label == nullptr) {
      continue;
    }

    const ScreenPoint anchor = project(label->anchor);
    const ScreenRect bounds = EstimateBounds(*label, anchor);
    if (!IsOnScreen(bounds, width, height)) {
      result.collision_boxes.push_back(LabelCollisionBox{
          .priority = label->priority,
          .dataset_path = label->dataset_path,
          .feature_id = label->feature_id,
          .object_class_acronym = label->object_class_acronym,
          .bounds = bounds,
          .important = label->important,
          .accepted = false,
      });
      continue;
    }

    bool blocked = false;
    for (const auto& accepted : accepted_bounds) {
      if (Overlaps(bounds, accepted)) {
        blocked = true;
        break;
      }
    }
    result.collision_boxes.push_back(LabelCollisionBox{
        .priority = label->priority,
        .dataset_path = label->dataset_path,
        .feature_id = label->feature_id,
        .object_class_acronym = label->object_class_acronym,
        .bounds = bounds,
        .important = label->important,
        .accepted = !blocked,
    });
    if (blocked) {
      continue;
    }

    accepted_bounds.push_back(bounds);
    const double text_height = static_cast<double>(std::max(label->style.size_px, 8));
    const double text_width =
        static_cast<double>(label->text.size()) * text_height * kApproxCharWidthFactor;
    const ScreenPoint origin = ComputeLabelOrigin(*label, anchor, text_width);
    result.placed_labels.push_back(PlacedLabel{
        .priority = label->priority,
        .dataset_path = label->dataset_path,
        .feature_id = label->feature_id,
        .object_class_acronym = label->object_class_acronym,
        .text = label->text,
        .style = label->style,
        .anchor = anchor,
        .origin = origin,
        .bounds = bounds,
        .important = label->important,
    });
  }

  return result;
}

std::vector<PlacedLabel> LayoutPortrayalLabels(
    const portrayal::PortrayalScene& scene,
    uint32_t width,
    uint32_t height,
    const std::function<ScreenPoint(const GeoPoint&)>& project) {
  return LayoutPortrayalLabelsDetailed(scene, width, height, project).placed_labels;
}

}  // namespace navscene::render
