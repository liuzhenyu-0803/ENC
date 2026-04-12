#include "portrayal/engine_internal.h"

#include <charconv>
#include <cmath>
#include <cstdio>

namespace navscene::portrayal::detail {

std::string TrimAscii(std::string_view value);
std::string FormatAttributeValue(std::string_view value,
                                 std::string_view prefix,
                                 std::string_view suffix);

namespace {

TextStyle DefaultLabelTextStyle(bool important,
                                std::string_view object_class_acronym,
                                const DisplaySettings& settings,
                                const S57PortrayalCatalog& catalog) {
  if (object_class_acronym == "SOUNDG") {
    return ResolveTextStyle(TextStyle{
        .role = FontRole::kSounding,
        .color = {7, 7, 7},
        .palette_color_id = "SNDG2",
        .size_px = 11,
        .halo = true,
    },
                            settings,
                            catalog);
  }

  return ResolveTextStyle(TextStyle{
      .role = important ? FontRole::kImportant : FontRole::kBody,
      .color = {7, 7, 7},
      .palette_color_id = "CHBLK",
      .size_px = important ? 12 : 11,
      .halo = true,
  },
                          settings,
                          catalog);
}

TextStyle ResolveLabelTextStyle(const LookupResult& lookup,
                                std::string_view object_class_acronym,
                                const DisplaySettings& settings,
                                const S57PortrayalCatalog& catalog) {
  if (lookup.text_role != nullptr) {
    return ResolveTextStyle(lookup.text_role->text, settings, catalog);
  }
  const bool important = lookup.rule != nullptr && lookup.rule->important_label;
  return DefaultLabelTextStyle(important, object_class_acronym, settings, catalog);
}

std::string AbbreviateLightCharacter(std::string_view value) {
  const std::string trimmed = TrimAscii(value);
  if (trimmed.empty()) {
    return {};
  }

  if (const auto code = ParseIntAttributeValue(trimmed); code.has_value()) {
    switch (*code) {
      case 1:
        return "F";
      case 2:
        return "Fl";
      case 3:
        return "LFl";
      case 4:
        return "Q";
      case 5:
        return "VQ";
      case 6:
        return "UQ";
      case 7:
        return "Iso";
      case 8:
        return "Oc";
      case 9:
        return "IQ";
      case 10:
        return "IVQ";
      case 11:
        return "IUQ";
      case 12:
        return "Mo";
      case 13:
        return "FFl";
      case 14:
        return "Fl+LFl";
      case 15:
        return "OcFl";
      case 16:
        return "FLFl";
      case 17:
        return "Al.Oc";
      case 18:
        return "Al.LFl";
      case 19:
        return "Al.Fl";
      case 20:
        return "Al.Gr";
      case 21:
        return "Q+LFl";
      case 22:
        return "VQ+LFl";
      case 23:
        return "UQ+LFl";
      case 24:
        return "Al";
      default:
        break;
    }
  }

  return trimmed;
}

std::string AbbreviateLightColor(std::string_view value) {
  const auto tokens = SplitAttributeValue(value);
  if (tokens.empty()) {
    return {};
  }

  std::string result;
  for (const auto& token : tokens) {
    if (const auto code = ParseIntAttributeValue(token); code.has_value()) {
      switch (*code) {
        case 1:
          result += 'W';
          continue;
        case 2:
          result += 'B';
          continue;
        case 3:
          result += 'R';
          continue;
        case 4:
          result += 'G';
          continue;
        case 5:
          result += "Bu";
          continue;
        case 6:
          result += 'Y';
          continue;
        case 7:
          result += "Gy";
          continue;
        case 8:
          result += "Br";
          continue;
        case 9:
          result += "Am";
          continue;
        case 11:
          result += "Vi";
          continue;
        case 12:
          result += "Or";
          continue;
        default:
          break;
      }
    }

    result += token;
  }
  return result;
}

std::string FormatLightValue(std::string_view value, std::string_view suffix) {
  const std::string trimmed = TrimAscii(value);
  if (trimmed.empty()) {
    return {};
  }
  if (trimmed.ends_with(suffix)) {
    return trimmed;
  }
  return trimmed + std::string(suffix);
}

std::string AbbreviateSeabedNature(std::string_view value) {
  const auto tokens = SplitAttributeValue(value);
  if (tokens.empty()) {
    return {};
  }

  std::string label;
  for (const auto& token : tokens) {
    std::string abbreviation;
    if (const auto code = ParseIntAttributeValue(token); code.has_value()) {
      switch (*code) {
        case 1:
          abbreviation = "M";
          break;
        case 2:
          abbreviation = "Cy";
          break;
        case 3:
          abbreviation = "Si";
          break;
        case 4:
          abbreviation = "S";
          break;
        case 5:
          abbreviation = "St";
          break;
        case 6:
          abbreviation = "G";
          break;
        case 7:
          abbreviation = "P";
          break;
        case 8:
          abbreviation = "Cb";
          break;
        case 9:
          abbreviation = "R";
          break;
        case 11:
          abbreviation = "R";
          break;
        case 14:
          abbreviation = "Co";
          break;
        case 17:
          abbreviation = "Sh";
          break;
        case 18:
          abbreviation = "R";
          break;
        default:
          break;
      }
    }

    if (abbreviation.empty()) {
      continue;
    }
    if (!label.empty()) {
      label += ' ';
    }
    label += abbreviation;
  }
  return label;
}

std::string ComposeBridgeLabel(const AttributeList& attributes) {
  const std::string_view object_name = FindAttributeValue(attributes, "OBJNAM");
  if (!object_name.empty()) {
    return std::string(object_name);
  }

  const std::string clearance =
      FormatAttributeValue(FindAttributeValue(attributes, "VERCLR"), "clr ", "");
  if (!clearance.empty()) {
    return clearance;
  }

  const std::string closed_clearance =
      FormatAttributeValue(FindAttributeValue(attributes, "VERCCL"), "clr cl ", "");
  if (!closed_clearance.empty()) {
    return closed_clearance;
  }

  return FormatAttributeValue(FindAttributeValue(attributes, "VERCOP"), "clr op ", "");
}

std::string ComposeSeabedAreaLabel(const AttributeList& attributes) {
  const auto watlev = FindFirstAttributeIntValue(attributes, "WATLEV");
  if (watlev.has_value() && *watlev == 4) {
    if (AttributeContainsIntValue(attributes, "NATSUR", 9) ||
        AttributeContainsIntValue(attributes, "NATSUR", 11) ||
        AttributeContainsIntValue(attributes, "NATSUR", 14)) {
      return {};
    }
  }

  return AbbreviateSeabedNature(FindAttributeValue(attributes, "NATSUR"));
}

std::string ComposeLightLabel(const AttributeList& attributes) {
  const std::string light_character =
      AbbreviateLightCharacter(FindAttributeValue(attributes, "LITCHR"));
  const std::string signal_group = TrimAscii(FindAttributeValue(attributes, "SIGGRP"));
  const std::string light_color = AbbreviateLightColor(FindAttributeValue(attributes, "COLOUR"));
  const std::string signal_period =
      FormatLightValue(FindAttributeValue(attributes, "SIGPER"), "s");
  const std::string nominal_range =
      FormatLightValue(FindAttributeValue(attributes, "VALNMR"), "M");

  std::string label;
  if (!light_character.empty()) {
    label += light_character;
  }
  if (!signal_group.empty()) {
    label += signal_group;
  }
  if (!light_color.empty()) {
    label += light_color;
  }
  if (!signal_period.empty()) {
    if (!label.empty()) {
      label += ' ';
    }
    label += signal_period;
  }
  if (!nominal_range.empty()) {
    if (!label.empty()) {
      label += ' ';
    }
    label += nominal_range;
  }
  return label;
}

std::string DecorateNamedLabelText(std::string_view object_class_acronym,
                                   std::string_view text) {
  if (text.empty()) {
    return {};
  }
  if (object_class_acronym == "OFSPLF" || object_class_acronym == "PRDARE") {
    return "Prod " + std::string(text);
  }
  if (IsBeaconObjectClass(object_class_acronym)) {
    return "bn " + std::string(text);
  }
  if (IsBuoyObjectClass(object_class_acronym)) {
    return "by " + std::string(text);
  }
  return std::string(text);
}

std::string FormatOrientationLabel(std::string_view value) {
  const std::string trimmed = TrimAscii(value);
  if (trimmed.empty()) {
    return {};
  }

  double parsed = 0.0;
  try {
    parsed = std::stod(trimmed);
  } catch (...) {
    return {};
  }

  long rounded = static_cast<long>(std::lround(parsed));
  rounded %= 360;
  if (rounded < 0) {
    rounded += 360;
  }

  char buffer[16] = {};
  std::snprintf(buffer, sizeof(buffer), "%03ld deg", rounded);
  return std::string(buffer);
}

std::string SelectLabelText(std::string_view object_class_acronym,
                            const AttributeList& attributes) {
  if (object_class_acronym == "SOUNDG") {
    const std::string_view sounding = FindAttributeValue(attributes, "VALSOU");
    if (!sounding.empty()) {
      return std::string(sounding);
    }
  }

  if (object_class_acronym == "OBSTRN" || object_class_acronym == "UWTROC" ||
      object_class_acronym == "WRECKS") {
    const std::string_view sounding = FindAttributeValue(attributes, "VALSOU");
    if (!sounding.empty()) {
      return std::string(sounding);
    }
  }

  if (IsStandaloneLightObjectClass(object_class_acronym)) {
    const std::string light_label = ComposeLightLabel(attributes);
    if (!light_label.empty()) {
      return light_label;
    }
  }

  if (object_class_acronym == "RECTRC") {
    const std::string orientation = FormatOrientationLabel(
        FindAttributeValue(attributes, "ORIENT"));
    if (!orientation.empty()) {
      return orientation;
    }
  }

  if (object_class_acronym == "SBDARE") {
    const std::string seabed_label = ComposeSeabedAreaLabel(attributes);
    if (!seabed_label.empty()) {
      return seabed_label;
    }
  }

  if (object_class_acronym == "BRIDGE") {
    const std::string bridge_label = ComposeBridgeLabel(attributes);
    if (!bridge_label.empty()) {
      return bridge_label;
    }
  }

  const std::string_view object_name = FindAttributeValue(attributes, "OBJNAM");
  if (!object_name.empty()) {
    return DecorateNamedLabelText(object_class_acronym, object_name);
  }

  const std::string_view national_name = FindAttributeValue(attributes, "NOBJNM");
  if (!national_name.empty()) {
    return DecorateNamedLabelText(object_class_acronym, national_name);
  }

  if (object_class_acronym == "DEPCNT") {
    const std::string_view contour = FindAttributeValue(attributes, "VALDCO");
    if (!contour.empty()) {
      return std::string(contour);
    }
  }

  return {};
}

}  // namespace

std::string TrimAscii(std::string_view value) {
  size_t begin = 0;
  while (begin < value.size() &&
         (value[begin] == ' ' || value[begin] == '\t' || value[begin] == '\r' ||
          value[begin] == '\n')) {
    ++begin;
  }

  size_t end = value.size();
  while (end > begin &&
         (value[end - 1] == ' ' || value[end - 1] == '\t' || value[end - 1] == '\r' ||
          value[end - 1] == '\n')) {
    --end;
  }

  return std::string(value.substr(begin, end - begin));
}

std::string FormatAttributeValue(std::string_view value,
                                 std::string_view prefix,
                                 std::string_view suffix) {
  const std::string trimmed = TrimAscii(value);
  if (trimmed.empty()) {
    return {};
  }

  std::string result;
  result.reserve(prefix.size() + trimmed.size() + suffix.size());
  result += prefix;
  result += trimmed;
  result += suffix;
  return result;
}

std::vector<std::string> SplitAttributeValue(std::string_view value) {
  std::string_view trimmed = value;
  while (!trimmed.empty() &&
         (trimmed.front() == ' ' || trimmed.front() == '\t' || trimmed.front() == '\r' ||
          trimmed.front() == '\n')) {
    trimmed.remove_prefix(1);
  }
  while (!trimmed.empty() &&
         (trimmed.back() == ' ' || trimmed.back() == '\t' || trimmed.back() == '\r' ||
          trimmed.back() == '\n')) {
    trimmed.remove_suffix(1);
  }
  if (trimmed.size() >= 3 && trimmed.front() == '(' && trimmed.back() == ')') {
    const size_t colon = trimmed.find(':');
    if (colon != std::string_view::npos && colon + 1 < trimmed.size() - 1) {
      trimmed = trimmed.substr(colon + 1, trimmed.size() - colon - 2);
    }
  }

  std::vector<std::string> parts;
  size_t begin = 0;
  while (begin <= trimmed.size()) {
    const size_t end = trimmed.find(',', begin);
    if (end == std::string_view::npos) {
      const std::string token = TrimAscii(trimmed.substr(begin));
      if (!token.empty()) {
        parts.push_back(token);
      }
      break;
    }

    const std::string token = TrimAscii(trimmed.substr(begin, end - begin));
    if (!token.empty()) {
      parts.push_back(token);
    }
    begin = end + 1;
  }
  return parts;
}

std::optional<int> ParseIntAttributeValue(std::string_view value) {
  if (value.empty()) {
    return std::nullopt;
  }

  int parsed = 0;
  const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (result.ec == std::errc{} && result.ptr == value.data() + value.size()) {
    return parsed;
  }
  return std::nullopt;
}

GeoPoint ComputePolylineAnchor(const render::PolylinePrimitive& polyline) {
  if (polyline.vertices.empty()) {
    return {};
  }
  return polyline.vertices[polyline.vertices.size() / 2];
}

GeoPoint ComputePolygonAnchor(const render::PolygonPrimitive& polygon) {
  if (polygon.outer_ring.empty()) {
    return {};
  }

  double min_lat = polygon.outer_ring.front().lat;
  double max_lat = polygon.outer_ring.front().lat;
  double min_lon = polygon.outer_ring.front().lon;
  double max_lon = polygon.outer_ring.front().lon;
  for (const auto& point : polygon.outer_ring) {
    min_lat = std::min(min_lat, point.lat);
    max_lat = std::max(max_lat, point.lat);
    min_lon = std::min(min_lon, point.lon);
    max_lon = std::max(max_lon, point.lon);
  }
  return GeoPoint{
      .lat = (min_lat + max_lat) * 0.5,
      .lon = (min_lon + max_lon) * 0.5,
  };
}

std::optional<LabelCandidate> BuildLabelCandidate(int priority,
                                                  const std::string& dataset_path,
                                                  int64_t feature_id,
                                                  const std::string& object_class_acronym,
                                                  const AttributeList& attributes,
                                                  const GeoPoint& anchor,
                                                  const LookupResult& lookup,
                                                  const DisplaySettings& settings,
                                                  const S57PortrayalCatalog& catalog) {
  if (!settings.show_text || lookup.rule == nullptr || !lookup.rule->generate_label) {
    return std::nullopt;
  }

  const std::string text = SelectLabelText(object_class_acronym, attributes);
  if (text.empty()) {
    return std::nullopt;
  }

  return LabelCandidate{
      .priority = priority + (lookup.rule->important_label ? 10 : 0),
      .dataset_path = dataset_path,
      .feature_id = feature_id,
      .object_class_acronym = object_class_acronym,
      .text = text,
      .anchor = anchor,
      .style = ResolveLabelTextStyle(lookup, object_class_acronym, settings, catalog),
      .important = lookup.rule->important_label,
  };
}

}  // namespace navscene::portrayal::detail
