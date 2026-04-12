#include "core/default_view.h"
#include "data/s57/reader.h"
#include "geo/mercator_projection.h"
#include "portrayal/engine.h"
#include "render/chart_scene_builder.h"
#include "render/software_raster.h"
#include "s57_reference_fixture.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

bool Expect(bool condition, std::string_view message) {
  if (condition) {
    return true;
  }

  std::cerr << "[navscene-s57-reference-render] " << message << '\n';
  return false;
}

uint64_t Fnv1a64(const std::vector<uint8_t>& bytes) {
  uint64_t hash = 1469598103934665603ull;
  for (uint8_t byte : bytes) {
    hash ^= static_cast<uint64_t>(byte);
    hash *= 1099511628211ull;
  }
  return hash;
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

std::optional<navscene::GeoPoint> ParseLonLat(std::string_view text) {
  const auto parts = Split(text, ',');
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

std::optional<double> ParseDouble(std::string_view text) {
  try {
    return std::stod(std::string(text));
  } catch (...) {
    return std::nullopt;
  }
}

void PrintUsage() {
  std::cerr << "usage:\n"
            << "  navscene-s57-reference-render [--center=<lon>,<lat>]\n"
            << "                               [--scale-ppm=<factor>]\n"
            << "                               [--display-scale=<denominator>]\n";
}

bool WriteBmp(const fs::path& path, const navscene::render::SoftwareRasterImage& image) {
  if (image.width == 0 || image.height == 0 || image.bgra_pixels.empty()) {
    return false;
  }

  const uint32_t pixel_bytes = image.width * image.height * 4u;
  const uint32_t file_size = 14u + 40u + pixel_bytes;
  std::ofstream stream(path, std::ios::binary);
  if (!stream.is_open()) {
    return false;
  }

  const std::array<uint8_t, 14> file_header = {
      static_cast<uint8_t>('B'),
      static_cast<uint8_t>('M'),
      static_cast<uint8_t>(file_size & 0xFF),
      static_cast<uint8_t>((file_size >> 8) & 0xFF),
      static_cast<uint8_t>((file_size >> 16) & 0xFF),
      static_cast<uint8_t>((file_size >> 24) & 0xFF),
      0,
      0,
      0,
      0,
      54,
      0,
      0,
      0,
  };
  stream.write(reinterpret_cast<const char*>(file_header.data()),
               static_cast<std::streamsize>(file_header.size()));

  const int32_t top_down_height = -static_cast<int32_t>(image.height);
  const std::array<uint8_t, 40> info_header = {
      40, 0, 0, 0,
      static_cast<uint8_t>(image.width & 0xFF),
      static_cast<uint8_t>((image.width >> 8) & 0xFF),
      static_cast<uint8_t>((image.width >> 16) & 0xFF),
      static_cast<uint8_t>((image.width >> 24) & 0xFF),
      static_cast<uint8_t>(top_down_height & 0xFF),
      static_cast<uint8_t>((top_down_height >> 8) & 0xFF),
      static_cast<uint8_t>((top_down_height >> 16) & 0xFF),
      static_cast<uint8_t>((top_down_height >> 24) & 0xFF),
      1, 0,
      32, 0,
      0, 0, 0, 0,
      static_cast<uint8_t>(pixel_bytes & 0xFF),
      static_cast<uint8_t>((pixel_bytes >> 8) & 0xFF),
      static_cast<uint8_t>((pixel_bytes >> 16) & 0xFF),
      static_cast<uint8_t>((pixel_bytes >> 24) & 0xFF),
      19, 11, 0, 0,
      19, 11, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0,
  };
  stream.write(reinterpret_cast<const char*>(info_header.data()),
               static_cast<std::streamsize>(info_header.size()));
  stream.write(reinterpret_cast<const char*>(image.bgra_pixels.data()),
               static_cast<std::streamsize>(image.bgra_pixels.size()));
  return stream.good();
}

template <typename Command>
void AccumulateObjectClasses(const std::vector<Command>& commands,
                             std::map<std::string, uint64_t>* counts) {
  if (counts == nullptr) {
    return;
  }
  for (const auto& command : commands) {
    counts->operator[](command.geometry.object_class_acronym) += 1;
  }
}

template <typename Command>
uint64_t CountObjectClass(const std::vector<Command>& commands, std::string_view object_class) {
  return static_cast<uint64_t>(std::count_if(commands.begin(),
                                             commands.end(),
                                             [&](const Command& command) {
                                               return command.geometry.object_class_acronym ==
                                                      object_class;
                                             }));
}

}  // namespace

int main(int argc, char** argv) {
#if !defined(NAVSCENE_HAS_GDAL) || !defined(_WIN32)
  std::cout << "[navscene-s57-reference-render] skipped: requires GDAL and Win32.\n";
  return 0;
#else
  std::optional<navscene::GeoPoint> center_override;
  std::optional<double> scale_ppm_override;
  std::optional<double> display_scale_override;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument.rfind("--center=", 0) == 0) {
      center_override = ParseLonLat(argument.substr(9));
      if (!center_override.has_value()) {
        PrintUsage();
        return 1;
      }
      continue;
    }
    if (argument.rfind("--scale-ppm=", 0) == 0) {
      scale_ppm_override = ParseDouble(argument.substr(12));
      if (!scale_ppm_override.has_value() || *scale_ppm_override <= 0.0) {
        PrintUsage();
        return 1;
      }
      continue;
    }
    if (argument.rfind("--display-scale=", 0) == 0) {
      display_scale_override = ParseDouble(argument.substr(16));
      if (!display_scale_override.has_value() || *display_scale_override <= 0.0) {
        PrintUsage();
        return 1;
      }
      continue;
    }

    PrintUsage();
    return 1;
  }

  const auto project_root = FindProjectRoot();
  if (!Expect(project_root.has_value(), "Could not resolve project root from current path.")) {
    return 1;
  }

  const fs::path dataset_path = *project_root / "data" / "s57" / "GB4X0000.000";
  if (!Expect(fs::exists(dataset_path), "Bundled GB4X0000 sample is missing.")) {
    return 1;
  }

  navscene::data::s57::DatasetInfo dataset;
  const auto load_status = navscene::data::s57::LoadDataset(
      navscene::data::s57::ReadRequest{dataset_path, navscene::SourceType::kChartDataset},
      &dataset);
  if (!Expect(load_status.ok(), "Bundled GB4X0000 sample should load successfully.")) {
    return 1;
  }

  navscene::render::ChartScene scene;
  navscene::render::AppendDatasetToChartScene(dataset, &scene);

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

  if (!Expect(!portrayal_scene.areas.empty(),
              "Reference render should produce portrayed area commands.")) {
    return 1;
  }
  if (!Expect(!portrayal_scene.lines.empty(),
              "Reference render should produce portrayed line commands.")) {
    return 1;
  }
  if (!Expect(CountObjectClass(portrayal_scene.points, "SOUNDG") == 0,
              "Default reference render should suppress soundings just like the demo.")) {
    return 1;
  }

  const navscene::GeoBox coverage = dataset.descriptor.coverage;
  navscene::Viewport viewport =
      navscene::testsupport::MakeBundledReferenceViewport(dataset.descriptor, 1280, 1024);
  if (display_scale_override.has_value()) {
    const navscene::GeoBox fitted_coverage =
        navscene::geo::ComputeViewportCoverage(coverage, viewport);
    const double fitted_display_scale =
        navscene::geo::EstimateViewportDisplayScale(fitted_coverage, viewport);
    if (!Expect(fitted_display_scale > 0.0,
                "Fitted viewport display scale must be measurable.")) {
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

  navscene::render::SoftwareRasterImage image;
  const auto raster_status = navscene::render::RasterizeChartSceneWin32(
      portrayal_scene, coverage, viewport, viewport.width, viewport.height, &image);
  if (!Expect(raster_status.ok(), "Reference render rasterization should succeed.")) {
    return 1;
  }

  const fs::path output_dir = fs::current_path() / "validation";
  fs::create_directories(output_dir);
  const fs::path bmp_path = output_dir / "GB4X0000-render.bmp";
  const fs::path report_path = output_dir / "GB4X0000-render-report.txt";
  if (!Expect(WriteBmp(bmp_path, image), "Reference render BMP export should succeed.")) {
    return 1;
  }

  std::map<std::string, uint64_t> class_counts;
  AccumulateObjectClasses(portrayal_scene.areas, &class_counts);
  AccumulateObjectClasses(portrayal_scene.lines, &class_counts);
  AccumulateObjectClasses(portrayal_scene.points, &class_counts);

  std::vector<std::pair<std::string, uint64_t>> sorted_counts(class_counts.begin(),
                                                              class_counts.end());
  std::sort(sorted_counts.begin(),
            sorted_counts.end(),
            [](const auto& lhs, const auto& rhs) {
              if (lhs.second != rhs.second) {
                return lhs.second > rhs.second;
              }
              return lhs.first < rhs.first;
            });

  std::ofstream report(report_path, std::ios::binary);
  if (!Expect(report.is_open(), "Reference render report file should be writable.")) {
    return 1;
  }

  report << "dataset=" << dataset_path.string() << '\n';
  report << "coverage=" << coverage.min_lon << ',' << coverage.min_lat << ','
         << coverage.max_lon << ',' << coverage.max_lat << '\n';
  report << "default_view_center=" << dataset.descriptor.default_view_center.lon << ','
         << dataset.descriptor.default_view_center.lat << '\n';
  report << "has_default_view_center="
         << (dataset.descriptor.has_default_view_center ? "true" : "false") << '\n';
  report << "default_display_scale=" << dataset.descriptor.default_display_scale << '\n';
  report << "viewport_center=" << viewport.center.lon << ',' << viewport.center.lat << '\n';
  report << "viewport_scale_ppm=" << viewport.scale_ppm << '\n';
  report << "viewport_display_scale="
         << navscene::geo::EstimateViewportDisplayScale(
                navscene::geo::ComputeViewportCoverage(coverage, viewport), viewport)
         << '\n';
  report << "image=" << bmp_path.string() << '\n';
  report << "pixel_hash=" << Fnv1a64(image.bgra_pixels) << '\n';
  report << "area_commands=" << portrayal_scene.stats.area_command_count << '\n';
  report << "area_overlays=" << portrayal_scene.stats.area_overlay_count << '\n';
  report << "line_commands=" << portrayal_scene.stats.line_command_count << '\n';
  report << "point_commands=" << portrayal_scene.stats.point_command_count << '\n';
  report << "label_candidates=" << portrayal_scene.stats.label_candidate_count << '\n';
  report << "top_classes:\n";
  for (size_t index = 0; index < std::min<size_t>(sorted_counts.size(), 24); ++index) {
    report << "  " << sorted_counts[index].first << '=' << sorted_counts[index].second << '\n';
  }

  std::cout << "[navscene-s57-reference-render] image=" << bmp_path.string() << '\n';
  std::cout << "[navscene-s57-reference-render] report=" << report_path.string() << '\n';
  std::cout << "[navscene-s57-reference-render] pixel_hash=" << Fnv1a64(image.bgra_pixels)
            << '\n';
  return 0;
#endif
}
