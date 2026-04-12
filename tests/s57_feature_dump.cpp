#include "core/default_view.h"
#include "data/s57/reader.h"
#include "data/s57/object_class_catalog.h"
#include "geo/mercator_projection.h"
#include "portrayal/engine.h"
#include "render/chart_scene_builder.h"
#include "s57_reference_fixture.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

namespace {

constexpr int kReferenceWidth = 1280;
constexpr int kReferenceHeight = 1024;
constexpr int kPaddingPixels = 0;

bool ContainsCaseInsensitive(std::string_view text, std::string_view needle) {
  if (needle.empty()) {
    return true;
  }
  if (needle.size() > text.size()) {
    return false;
  }

  auto ascii_lower = [](unsigned char ch) -> char {
    return static_cast<char>(std::tolower(ch));
  };

  for (size_t index = 0; index + needle.size() <= text.size(); ++index) {
    bool matched = true;
    for (size_t offset = 0; offset < needle.size(); ++offset) {
      if (ascii_lower(static_cast<unsigned char>(text[index + offset])) !=
          ascii_lower(static_cast<unsigned char>(needle[offset]))) {
        matched = false;
        break;
      }
    }
    if (matched) {
      return true;
    }
  }
  return false;
}

std::optional<fs::path> FindProjectRoot() {
  fs::path current = fs::current_path();
  for (int depth = 0; depth < 8; ++depth) {
    if (fs::exists(current / "data" / "s57" / "GB4X0000.000")) {
      return current;
    }
    if (!current.has_parent_path()) {
      break;
    }
    current = current.parent_path();
  }
  return std::nullopt;
}

std::vector<std::string> Split(std::string_view text, char delimiter) {
  std::vector<std::string> parts;
  size_t begin = 0;
  while (begin <= text.size()) {
    const size_t end = text.find(delimiter, begin);
    if (end == std::string_view::npos) {
      if (begin < text.size()) {
        parts.emplace_back(text.substr(begin));
      }
      break;
    }
    if (end > begin) {
      parts.emplace_back(text.substr(begin, end - begin));
    }
    begin = end + 1;
  }
  return parts;
}

template <typename Feature>
void DumpFeatureList(const std::vector<Feature>& features,
                     std::string_view object_class,
                     std::map<std::string, uint64_t>* attribute_counts) {
  for (const auto& feature : features) {
    if (feature.object_class_acronym != object_class) {
      continue;
    }

    std::cout << "feature " << feature.feature_id << " class=" << feature.object_class_acronym
              << " layer=" << feature.source_layer << '\n';
    for (const auto& [key, value] : feature.attributes) {
      std::cout << "  " << key << "=" << value << '\n';
      for (const auto& token : Split(value, ',')) {
        attribute_counts->operator[](key + "=" + token) += 1;
      }
    }
    std::cout << '\n';
  }
}

template <typename Feature>
std::optional<navscene::GeoPoint> ComputeAnchor(const Feature& feature) {
  return std::nullopt;
}

template <>
std::optional<navscene::GeoPoint> ComputeAnchor(
    const navscene::data::s57::PointFeatureGeometry& feature) {
  if (feature.points.empty()) {
    return std::nullopt;
  }
  return feature.points.front().position;
}

template <>
std::optional<navscene::GeoPoint> ComputeAnchor(
    const navscene::data::s57::LineFeatureGeometry& feature) {
  if (feature.parts.empty() || feature.parts.front().vertices.empty()) {
    return std::nullopt;
  }
  const auto& vertices = feature.parts.front().vertices;
  return vertices[vertices.size() / 2];
}

template <>
std::optional<navscene::GeoPoint> ComputeAnchor(
    const navscene::data::s57::AreaFeatureGeometry& feature) {
  if (feature.polygons.empty() || feature.polygons.front().outer_ring.empty()) {
    return std::nullopt;
  }

  const auto& ring = feature.polygons.front().outer_ring;
  double min_lat = ring.front().lat;
  double max_lat = ring.front().lat;
  double min_lon = ring.front().lon;
  double max_lon = ring.front().lon;
  for (const auto& point : ring) {
    min_lat = std::min(min_lat, point.lat);
    max_lat = std::max(max_lat, point.lat);
    min_lon = std::min(min_lon, point.lon);
    max_lon = std::max(max_lon, point.lon);
  }
  return navscene::GeoPoint{
      .lat = (min_lat + max_lat) * 0.5,
      .lon = (min_lon + max_lon) * 0.5,
  };
}

template <typename Feature>
bool FeatureMatchesName(const Feature& feature, std::string_view needle) {
  for (const auto& [key, value] : feature.attributes) {
    if ((key == "OBJNAM" || key == "NOBJNM") && ContainsCaseInsensitive(value, needle)) {
      return true;
    }
  }
  return false;
}

navscene::Viewport BuildReferenceViewport(const navscene::DatasetDescriptor& descriptor) {
  return navscene::testsupport::MakeBundledReferenceViewport(
      descriptor, kReferenceWidth, kReferenceHeight, kPaddingPixels);
}

navscene::GeoPoint PixelToGeo(const navscene::GeoBox& coverage,
                              const navscene::Viewport& viewport,
                              double pixel_x,
                              double pixel_y) {
  return navscene::geo::PixelToGeo(coverage, viewport, pixel_x, pixel_y, kPaddingPixels);
}

std::pair<double, double> GeoToPixel(const navscene::GeoBox& coverage,
                                     const navscene::Viewport& viewport,
                                     const navscene::GeoPoint& point) {
  return navscene::geo::GeoToPixel(coverage, viewport, point, kPaddingPixels);
}

bool RingContainsPoint(const std::vector<navscene::GeoPoint>& ring,
                       const navscene::GeoPoint& point) {
  if (ring.size() < 3) {
    return false;
  }

  bool inside = false;
  for (size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++) {
    const auto& a = ring[i];
    const auto& b = ring[j];
    const bool crosses = ((a.lat > point.lat) != (b.lat > point.lat)) &&
                         (point.lon < (b.lon - a.lon) * (point.lat - a.lat) /
                                              ((b.lat - a.lat) == 0.0 ? 1e-12 : (b.lat - a.lat)) +
                                          a.lon);
    if (crosses) {
      inside = !inside;
    }
  }
  return inside;
}

bool PolygonContainsPoint(const navscene::render::PolygonPrimitive& polygon,
                          const navscene::GeoPoint& point) {
  if (!RingContainsPoint(polygon.outer_ring, point)) {
    return false;
  }
  for (const auto& hole : polygon.holes) {
    if (RingContainsPoint(hole, point)) {
      return false;
    }
  }
  return true;
}

navscene::GeoBox ComputePolygonBounds(const navscene::render::PolygonPrimitive& polygon) {
  navscene::GeoBox bounds{};
  if (polygon.outer_ring.empty()) {
    return bounds;
  }

  bounds.min_lat = bounds.max_lat = polygon.outer_ring.front().lat;
  bounds.min_lon = bounds.max_lon = polygon.outer_ring.front().lon;
  for (const auto& point : polygon.outer_ring) {
    bounds.min_lat = std::min(bounds.min_lat, point.lat);
    bounds.max_lat = std::max(bounds.max_lat, point.lat);
    bounds.min_lon = std::min(bounds.min_lon, point.lon);
    bounds.max_lon = std::max(bounds.max_lon, point.lon);
  }
  return bounds;
}

bool BoundsContainPoint(const navscene::GeoBox& bounds, const navscene::GeoPoint& point) {
  return point.lat >= bounds.min_lat && point.lat <= bounds.max_lat &&
         point.lon >= bounds.min_lon && point.lon <= bounds.max_lon;
}

double SquaredDistance(double dx, double dy) {
  return dx * dx + dy * dy;
}

double DistancePointToSegmentSquared(double px,
                                     double py,
                                     double ax,
                                     double ay,
                                     double bx,
                                     double by) {
  const double abx = bx - ax;
  const double aby = by - ay;
  const double ab_length_squared = SquaredDistance(abx, aby);
  if (ab_length_squared <= 1e-12) {
    return SquaredDistance(px - ax, py - ay);
  }

  const double apx = px - ax;
  const double apy = py - ay;
  const double t = std::clamp((apx * abx + apy * aby) / ab_length_squared, 0.0, 1.0);
  const double closest_x = ax + abx * t;
  const double closest_y = ay + aby * t;
  return SquaredDistance(px - closest_x, py - closest_y);
}

void DumpRawNameMatches(const navscene::data::s57::DatasetInfo& dataset,
                        const navscene::GeoBox& coverage,
                        const navscene::Viewport& viewport,
                        std::string_view needle) {
  auto dump = [&](const auto& features, std::string_view kind) {
    for (const auto& feature : features) {
      if (!FeatureMatchesName(feature, needle)) {
        continue;
      }
      const auto anchor = ComputeAnchor(feature);
      std::cout << "raw-match kind=" << kind << " class=" << feature.object_class_acronym
                << " fid=" << feature.feature_id << " layer=" << feature.source_layer;
      if (anchor.has_value()) {
        const auto [pixel_x, pixel_y] = GeoToPixel(coverage, viewport, *anchor);
        std::cout << " anchor=(" << anchor->lon << "," << anchor->lat << ")"
                  << " pixel=(" << std::lround(pixel_x) << "," << std::lround(pixel_y) << ")";
      }
      std::cout << '\n';
      for (const auto& [key, value] : feature.attributes) {
        if (key == "OBJNAM" || key == "NOBJNM") {
          std::cout << "  " << key << "=" << value << '\n';
        }
      }
    }
  };

  dump(dataset.geometry.point_features, "point");
  dump(dataset.geometry.line_features, "line");
  dump(dataset.geometry.area_features, "area");
}

void DumpLabelMatches(const navscene::portrayal::PortrayalScene& portrayal_scene,
                      const navscene::GeoBox& coverage,
                      const navscene::Viewport& viewport,
                      std::string_view needle) {
  for (const auto& label : portrayal_scene.labels) {
    if (!ContainsCaseInsensitive(label.text, needle)) {
      continue;
    }
    const auto [pixel_x, pixel_y] = GeoToPixel(coverage, viewport, label.anchor);
    std::cout << "label text=\"" << label.text << "\" class=" << label.object_class_acronym
              << " fid=" << label.feature_id << " priority=" << label.priority
              << " anchor=(" << label.anchor.lon << "," << label.anchor.lat << ")"
              << " pixel=(" << std::lround(pixel_x) << "," << std::lround(pixel_y) << ")\n";
  }
}

void DumpAreaCoverageAtPoint(const navscene::portrayal::PortrayalScene& portrayal_scene,
                             const navscene::GeoBox& coverage,
                             const navscene::Viewport& viewport,
                             const navscene::GeoPoint& point) {
  const auto [pixel_x, pixel_y] = GeoToPixel(coverage, viewport, point);
  std::cout << "probe lon=" << point.lon << " lat=" << point.lat
            << " pixel=(" << std::lround(pixel_x) << "," << std::lround(pixel_y) << ")\n";

  struct Candidate {
    const navscene::portrayal::AreaCommand* area = nullptr;
    navscene::GeoBox bounds;
    bool contains = false;
    double bounds_area = 0.0;
  };

  std::vector<Candidate> candidates;
  for (const auto& area : portrayal_scene.areas) {
    const navscene::GeoBox bounds = ComputePolygonBounds(area.geometry);
    if (!BoundsContainPoint(bounds, point)) {
      continue;
    }
    candidates.push_back(Candidate{
        .area = &area,
        .bounds = bounds,
        .contains = PolygonContainsPoint(area.geometry, point),
        .bounds_area = (bounds.max_lon - bounds.min_lon) * (bounds.max_lat - bounds.min_lat),
    });
  }

  std::sort(candidates.begin(),
            candidates.end(),
            [](const Candidate& lhs, const Candidate& rhs) {
              if (lhs.contains != rhs.contains) {
                return lhs.contains > rhs.contains;
              }
              if (lhs.bounds_area != rhs.bounds_area) {
                return lhs.bounds_area < rhs.bounds_area;
              }
              if (lhs.area->priority != rhs.area->priority) {
                return lhs.area->priority < rhs.area->priority;
              }
              return lhs.area->geometry.feature_id < rhs.area->geometry.feature_id;
            });

  std::cout << "bbox-candidates=" << candidates.size() << '\n';
  for (size_t index = 0; index < std::min<size_t>(candidates.size(), 20); ++index) {
    const auto& candidate = candidates[index];
    const auto& area = *candidate.area;

    std::cout << "area class=" << area.geometry.object_class_acronym
              << " fid=" << area.geometry.feature_id << " priority=" << area.priority
              << " contains=" << (candidate.contains ? "yes" : "no")
              << " bbox=(" << candidate.bounds.min_lon << ',' << candidate.bounds.min_lat << ")-("
              << candidate.bounds.max_lon << ',' << candidate.bounds.max_lat << ')'
              << " fill=" << (area.fill.enabled ? "on" : "off") << '['
              << static_cast<int>(area.fill.color.r) << ','
              << static_cast<int>(area.fill.color.g) << ','
              << static_cast<int>(area.fill.color.b) << "]"
              << " stroke=" << (area.stroke.enabled ? "on" : "off") << '['
              << static_cast<int>(area.stroke.color.r) << ','
              << static_cast<int>(area.stroke.color.g) << ','
              << static_cast<int>(area.stroke.color.b) << "]\n";

    for (const auto& [key, value] : area.geometry.attributes) {
      if (key == "OBJNAM" || key == "NOBJNM" || key == "DRVAL1" || key == "DRVAL2" ||
          key == "WATLEV" || key == "CATOBS") {
        std::cout << "  " << key << "=" << value << '\n';
      }
    }
  }
}

void DumpLineAndPointCoverageAtPixel(const navscene::portrayal::PortrayalScene& portrayal_scene,
                                     const navscene::GeoBox& coverage,
                                     const navscene::Viewport& viewport,
                                     double pixel_x,
                                     double pixel_y) {
  struct LineCandidate {
    const navscene::portrayal::LineCommand* line = nullptr;
    double distance_squared = 0.0;
  };
  struct PointCandidate {
    const navscene::portrayal::PointCommand* point = nullptr;
    double distance_squared = 0.0;
    double projected_x = 0.0;
    double projected_y = 0.0;
  };

  std::vector<LineCandidate> line_candidates;
  for (const auto& line : portrayal_scene.lines) {
    if (line.geometry.vertices.size() < 2 || !line.visible || !line.stroke.enabled) {
      continue;
    }

    double best_distance_squared = std::numeric_limits<double>::max();
    for (size_t index = 1; index < line.geometry.vertices.size(); ++index) {
      const auto [ax, ay] = GeoToPixel(coverage, viewport, line.geometry.vertices[index - 1]);
      const auto [bx, by] = GeoToPixel(coverage, viewport, line.geometry.vertices[index]);
      best_distance_squared = std::min(
          best_distance_squared, DistancePointToSegmentSquared(pixel_x, pixel_y, ax, ay, bx, by));
    }
    if (best_distance_squared <= 9.0) {
      line_candidates.push_back(LineCandidate{
          .line = &line,
          .distance_squared = best_distance_squared,
      });
    }
  }

  std::sort(line_candidates.begin(),
            line_candidates.end(),
            [](const LineCandidate& lhs, const LineCandidate& rhs) {
              if (lhs.distance_squared != rhs.distance_squared) {
                return lhs.distance_squared < rhs.distance_squared;
              }
              if (lhs.line->priority != rhs.line->priority) {
                return lhs.line->priority < rhs.line->priority;
              }
              return lhs.line->geometry.feature_id < rhs.line->geometry.feature_id;
            });

  std::cout << "line-candidates=" << line_candidates.size() << '\n';
  for (size_t index = 0; index < std::min<size_t>(line_candidates.size(), 8); ++index) {
    const auto& candidate = line_candidates[index];
    const auto& line = *candidate.line;
    std::cout << "line class=" << line.geometry.object_class_acronym
              << " fid=" << line.geometry.feature_id << " priority=" << line.priority
              << " dist_px=" << std::sqrt(candidate.distance_squared)
              << " stroke=on[" << static_cast<int>(line.stroke.color.r) << ','
              << static_cast<int>(line.stroke.color.g) << ','
              << static_cast<int>(line.stroke.color.b) << "] width=" << line.stroke.width_px
              << '\n';
  }

  std::vector<PointCandidate> point_candidates;
  for (const auto& point : portrayal_scene.points) {
    if (!point.visible || !point.symbol.enabled) {
      continue;
    }

    const auto [projected_x, projected_y] = GeoToPixel(coverage, viewport, point.geometry.position);
    const double distance_squared =
        SquaredDistance(pixel_x - projected_x, pixel_y - projected_y);
    const double radius = std::max(point.symbol.size_px * 0.5 + 2.0, 5.0);
    if (distance_squared <= radius * radius) {
      point_candidates.push_back(PointCandidate{
          .point = &point,
          .distance_squared = distance_squared,
          .projected_x = projected_x,
          .projected_y = projected_y,
      });
    }
  }

  std::sort(point_candidates.begin(),
            point_candidates.end(),
            [](const PointCandidate& lhs, const PointCandidate& rhs) {
              if (lhs.distance_squared != rhs.distance_squared) {
                return lhs.distance_squared < rhs.distance_squared;
              }
              if (lhs.point->priority != rhs.point->priority) {
                return lhs.point->priority < rhs.point->priority;
              }
              return lhs.point->geometry.feature_id < rhs.point->geometry.feature_id;
            });

  std::cout << "point-candidates=" << point_candidates.size() << '\n';
  for (size_t index = 0; index < std::min<size_t>(point_candidates.size(), 8); ++index) {
    const auto& candidate = point_candidates[index];
    const auto& point = *candidate.point;
    std::cout << "point class=" << point.geometry.object_class_acronym
              << " fid=" << point.geometry.feature_id << " priority=" << point.priority
              << " dist_px=" << std::sqrt(candidate.distance_squared)
              << " anchor=(" << std::lround(candidate.projected_x) << ','
              << std::lround(candidate.projected_y) << ")"
              << " fill=[" << static_cast<int>(point.symbol.fill.r) << ','
              << static_cast<int>(point.symbol.fill.g) << ','
              << static_cast<int>(point.symbol.fill.b) << "]"
              << " stroke=[" << static_cast<int>(point.symbol.stroke.r) << ','
              << static_cast<int>(point.symbol.stroke.g) << ','
              << static_cast<int>(point.symbol.stroke.b) << "]"
              << " size=" << point.symbol.size_px << '\n';
  }
}

std::optional<navscene::GeoPoint> ParseLonLatArgument(const std::string& value) {
  const auto parts = Split(value, ',');
  if (parts.size() != 2) {
    return std::nullopt;
  }
  try {
    return navscene::GeoPoint{
        .lat = std::stod(parts[1]),
        .lon = std::stod(parts[0]),
    };
  } catch (...) {
    return std::nullopt;
  }
}

std::optional<std::pair<double, double>> ParsePixelArgument(const std::string& value) {
  const auto parts = Split(value, ',');
  if (parts.size() != 2) {
    return std::nullopt;
  }
  try {
    return std::pair<double, double>{std::stod(parts[0]), std::stod(parts[1])};
  } catch (...) {
    return std::nullopt;
  }
}

void PrintUsage() {
  std::cerr << "usage:\n"
            << "  navscene-s57-feature-dump [viewport options] <OBJECT_CLASS> [OBJECT_CLASS...]\n"
            << "  navscene-s57-feature-dump [viewport options] --name=<text>\n"
            << "  navscene-s57-feature-dump [viewport options] --list-areas\n"
            << "  navscene-s57-feature-dump [viewport options] --pixel=<x>,<y>\n"
            << "  navscene-s57-feature-dump [viewport options] --lonlat=<lon>,<lat>\n"
            << "viewport options:\n"
            << "  --center=<lon>,<lat>\n"
            << "  --scale-ppm=<factor>\n"
            << "  --display-scale=<denominator>\n";
}

}  // namespace

int main(int argc, char** argv) {
#if !defined(NAVSCENE_HAS_GDAL)
  std::cout << "requires GDAL\n";
  return 0;
#else
  if (argc < 2) {
    PrintUsage();
    return 1;
  }

  const auto project_root = FindProjectRoot();
  if (!project_root.has_value()) {
    std::cerr << "could not resolve project root\n";
    return 1;
  }

  std::optional<navscene::GeoPoint> center_override;
  std::optional<double> scale_ppm_override;
  std::optional<double> display_scale_override;
  std::optional<std::string> name_query;
  std::optional<std::pair<double, double>> pixel_query;
  std::optional<navscene::GeoPoint> lonlat_query;
  bool list_areas = false;
  std::vector<std::string> object_classes;

  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument.rfind("--center=", 0) == 0) {
      center_override = ParseLonLatArgument(argument.substr(9));
      if (!center_override.has_value()) {
        PrintUsage();
        return 1;
      }
      continue;
    }
    if (argument.rfind("--scale-ppm=", 0) == 0) {
      try {
        scale_ppm_override = std::stod(argument.substr(12));
      } catch (...) {
        scale_ppm_override.reset();
      }
      if (!scale_ppm_override.has_value() || *scale_ppm_override <= 0.0) {
        PrintUsage();
        return 1;
      }
      continue;
    }
    if (argument.rfind("--display-scale=", 0) == 0) {
      try {
        display_scale_override = std::stod(argument.substr(16));
      } catch (...) {
        display_scale_override.reset();
      }
      if (!display_scale_override.has_value() || *display_scale_override <= 0.0) {
        PrintUsage();
        return 1;
      }
      continue;
    }
    if (argument.rfind("--name=", 0) == 0) {
      name_query = argument.substr(7);
      continue;
    }
    if (argument.rfind("--pixel=", 0) == 0) {
      pixel_query = ParsePixelArgument(argument.substr(8));
      if (!pixel_query.has_value()) {
        PrintUsage();
        return 1;
      }
      continue;
    }
    if (argument == "--list-areas") {
      list_areas = true;
      continue;
    }
    if (argument.rfind("--lonlat=", 0) == 0) {
      lonlat_query = ParseLonLatArgument(argument.substr(9));
      if (!lonlat_query.has_value()) {
        PrintUsage();
        return 1;
      }
      continue;
    }

    object_classes.push_back(argument);
  }

  navscene::data::s57::DatasetInfo dataset;
  const auto load_status = navscene::data::s57::LoadDataset(
      navscene::data::s57::ReadRequest{*project_root / "data" / "s57" / "GB4X0000.000",
                                       navscene::SourceType::kChartDataset},
      &dataset);
  if (!load_status.ok()) {
    std::cerr << "load failed: " << load_status.message << '\n';
    return 1;
  }

  const navscene::GeoBox coverage = dataset.descriptor.coverage;
  navscene::Viewport viewport = BuildReferenceViewport(dataset.descriptor);
  if (display_scale_override.has_value()) {
    const navscene::GeoBox fitted_coverage =
        navscene::geo::ComputeViewportCoverage(coverage, viewport, kPaddingPixels);
    const double fitted_display_scale =
        navscene::geo::EstimateViewportDisplayScale( fitted_coverage, viewport, kPaddingPixels);
    if (fitted_display_scale <= 0.0) {
      std::cerr << "could not estimate fitted display scale\n";
      return 1;
    }
    viewport.scale_ppm = fitted_display_scale / *display_scale_override;
  }
  if (scale_ppm_override.has_value()) {
    viewport.scale_ppm = *scale_ppm_override;
  }
  if (center_override.has_value()) {
    viewport.center = *center_override;
  }

  navscene::render::ChartScene scene;
  navscene::render::AppendDatasetToChartScene(dataset, &scene);
  std::map<int, int> polygon_codes;
  for (const auto& polygon : scene.polygons) {
    polygon_codes[polygon.object_class_code] += 1;
  }
  navscene::DisplayOptions display_options;
  display_options.show_text = false;
  display_options.show_soundings = false;
  display_options.show_meta = false;
  display_options.show_quality_of_data = false;
  display_options.display_category = navscene::DisplayCategory::kStandard;
  display_options.simplified_points = true;
  display_options.symbolized_boundaries = false;
  display_options.safety_contour_m = 10.0;
  display_options.safety_depth_m = 7.0;
  display_options.shallow_contour_m = 5.0;
  display_options.deep_contour_m = 20.0;
  const auto settings = navscene::portrayal::MakeDisplaySettings(display_options);
  const auto portrayal_scene =
      navscene::portrayal::BuildPortrayalScene(scene, settings,
                                               navscene::portrayal::PortrayalProfileId::kS57);
  std::cout << "scene-counts polygons=" << scene.polygons.size()
            << " polylines=" << scene.polylines.size()
            << " points=" << scene.points.size()
            << " portrayed-areas=" << portrayal_scene.areas.size()
            << " portrayed-lines=" << portrayal_scene.lines.size()
            << " portrayed-points=" << portrayal_scene.points.size()
            << " labels=" << portrayal_scene.labels.size() << '\n';
  std::cout << "polygon-code-sample";
  int emitted_codes = 0;
  for (const auto& [code, count] : polygon_codes) {
    if (emitted_codes >= 8) {
      break;
    }
    std::cout << ' ' << code << ':' << count << ':';
    if (const auto* info = navscene::data::s57::FindObjectClassInfo(code)) {
      std::cout << info->acronym;
    } else {
      std::cout << "(null)";
    }
    ++emitted_codes;
  }
  std::cout << '\n';

  if (name_query.has_value()) {
    DumpRawNameMatches(dataset, coverage, viewport, *name_query);
    DumpLabelMatches(portrayal_scene, coverage, viewport, *name_query);
    return 0;
  }

  if (pixel_query.has_value()) {
    const auto point = PixelToGeo(coverage, viewport, pixel_query->first, pixel_query->second);
    DumpAreaCoverageAtPoint(portrayal_scene, coverage, viewport, point);
    DumpLineAndPointCoverageAtPixel(
        portrayal_scene, coverage, viewport, pixel_query->first, pixel_query->second);
    return 0;
  }

  if (list_areas) {
    for (size_t index = 0; index < std::min<size_t>(portrayal_scene.areas.size(), 40); ++index) {
      const auto& area = portrayal_scene.areas[index];
      const auto bounds = ComputePolygonBounds(area.geometry);
      std::cout << index << " class=" << area.geometry.object_class_acronym
                << " fid=" << area.geometry.feature_id << " priority=" << area.priority
                << " bbox=(" << bounds.min_lon << ',' << bounds.min_lat << ")-("
                << bounds.max_lon << ',' << bounds.max_lat << ")\n";
    }
    return 0;
  }

  if (lonlat_query.has_value()) {
    DumpAreaCoverageAtPoint(portrayal_scene, coverage, viewport, *lonlat_query);
    const auto [pixel_x, pixel_y] = GeoToPixel(coverage, viewport, *lonlat_query);
    DumpLineAndPointCoverageAtPixel(portrayal_scene, coverage, viewport, pixel_x, pixel_y);
    return 0;
  }

  if (object_classes.empty()) {
    PrintUsage();
    return 1;
  }

  for (const std::string& object_class : object_classes) {
    std::map<std::string, uint64_t> attribute_counts;
    std::cout << "=== " << object_class << " ===\n";
    DumpFeatureList(dataset.geometry.point_features, object_class, &attribute_counts);
    DumpFeatureList(dataset.geometry.line_features, object_class, &attribute_counts);
    DumpFeatureList(dataset.geometry.area_features, object_class, &attribute_counts);

    std::vector<std::pair<std::string, uint64_t>> sorted(attribute_counts.begin(),
                                                         attribute_counts.end());
    std::sort(sorted.begin(),
              sorted.end(),
              [](const auto& lhs, const auto& rhs) {
                if (lhs.second != rhs.second) {
                  return lhs.second > rhs.second;
                }
                return lhs.first < rhs.first;
              });

    std::cout << "--- histogram ---\n";
    for (const auto& [entry, count] : sorted) {
      std::cout << count << "  " << entry << '\n';
    }
    std::cout << '\n';
  }

  return 0;
#endif
}
