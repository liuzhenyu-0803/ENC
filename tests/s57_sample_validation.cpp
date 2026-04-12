#include "navscene/navscene.h"
#include "core/default_view.h"
#include "data/s57/reader.h"
#include "geo/mercator_projection.h"
#include "render/chart_scene_builder.h"
#include "render/scene_signature.h"
#include "render/scene_svg_export.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>

namespace fs = std::filesystem;

namespace {

bool Expect(bool condition, std::string_view message) {
  if (condition) {
    return true;
  }

  std::cerr << "[navscene-s57-samples] " << message << '\n';
  return false;
}

bool ExpectNear(double value, double expected, double tolerance, std::string_view message) {
  if (std::abs(value - expected) <= tolerance) {
    return true;
  }

  std::cerr << "[navscene-s57-samples] " << message << " value=" << value
            << " expected=" << expected << " tolerance=" << tolerance << '\n';
  return false;
}

uint64_t Fnv1a64(std::string_view text) {
  uint64_t hash = 1469598103934665603ull;
  for (const unsigned char ch : text) {
    hash ^= ch;
    hash *= 1099511628211ull;
  }
  return hash;
}

fs::path ResolveSampleRoot() {
  if (const char* env = std::getenv("NAVSCENE_S57_SAMPLE_ROOT")) {
    if (*env != '\0') {
      return fs::path(env);
    }
  }
  return fs::path("E:/projects/enc/enc");
}

std::optional<navscene::DatasetDescriptor> FindDataset(
    const std::vector<navscene::DatasetDescriptor>& datasets,
    std::string_view suffix) {
  for (const auto& dataset : datasets) {
    if (dataset.path.ends_with(suffix)) {
      return dataset;
    }
  }
  return std::nullopt;
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

template <typename Primitive>
bool ContainsObjectClass(const std::vector<Primitive>& primitives,
                         std::string_view expected_acronym) {
  return std::any_of(primitives.begin(),
                     primitives.end(),
                     [&](const Primitive& primitive) {
                       return primitive.object_class_acronym == expected_acronym;
                     });
}

int ValidateDirectDataset(navscene::IEngine* engine, const fs::path& dataset_path) {
  navscene::SourceDescriptor descriptor;
  descriptor.id = "sample-direct-us2arceb";
  descriptor.uri = dataset_path.string();
  descriptor.type = navscene::SourceType::kChartDataset;
  descriptor.format = navscene::DatasetFormat::kS57;

  const auto register_status = engine->Sources().RegisterSource(descriptor);
  if (!Expect(register_status.ok(),
              "RegisterSource should succeed for the real US2ARCEB sample.")) {
    return 1;
  }

  const auto dataset = FindDataset(engine->Catalog().Snapshot(), "US2ARCEB.000");
  if (!Expect(dataset.has_value(),
              "Catalog snapshot should contain the direct US2ARCEB sample.")) {
    return 1;
  }

  if (!Expect(dataset->format == navscene::DatasetFormat::kS57,
              "US2ARCEB should be recognized as S-57.")) {
    return 1;
  }
  if (!Expect(dataset->family == navscene::ProductFamily::kEnc,
              "US2ARCEB should be recognized as ENC.")) {
    return 1;
  }
  if (!Expect(dataset->compilation_scale == 1500000,
              "US2ARCEB should expose the expected compilation scale.")) {
    return 1;
  }
  if (!ExpectNear(dataset->coverage.min_lon, -175.650537, 0.01,
                  "US2ARCEB min_lon should match the GDAL sample extent.")) {
    return 1;
  }
  if (!ExpectNear(dataset->coverage.min_lat, 62.4, 0.01,
                  "US2ARCEB min_lat should match the GDAL sample extent.")) {
    return 1;
  }
  if (!ExpectNear(dataset->coverage.max_lon, -172.8, 0.01,
                  "US2ARCEB max_lon should match the GDAL sample extent.")) {
    return 1;
  }
  if (!ExpectNear(dataset->coverage.max_lat, 63.768197, 0.01,
                  "US2ARCEB max_lat should match the GDAL sample extent.")) {
    return 1;
  }

  return 0;
}

int ValidateNormalizedGeometry(const fs::path& dataset_path,
                               uint64_t expected_points,
                               uint64_t expected_lines,
                               uint64_t expected_areas,
                               uint64_t expected_scene_points,
                               uint64_t expected_scene_lines,
                               uint64_t expected_scene_polygons,
                               uint64_t expected_scene_fingerprint,
                               uint64_t expected_svg_fingerprint,
                               std::string_view expected_point_class,
                               std::string_view expected_line_class,
                               std::string_view expected_area_class,
                               std::string_view label) {
  navscene::data::s57::DatasetInfo dataset;
  const auto load_status = navscene::data::s57::LoadDataset(
      navscene::data::s57::ReadRequest{dataset_path, navscene::SourceType::kChartDataset},
      &dataset);
  if (!Expect(load_status.ok(), std::string(label) + " geometry load should succeed.")) {
    return 1;
  }

  if (!Expect(dataset.geometry_loaded,
              std::string(label) + " should report loaded geometry.")) {
    return 1;
  }
  if (dataset.geometry.summary.point_feature_count != expected_points ||
      dataset.geometry.summary.line_feature_count != expected_lines ||
      dataset.geometry.summary.area_feature_count != expected_areas) {
    std::cerr << "[navscene-s57-samples] " << label
              << " actual geometry counts: points="
              << dataset.geometry.summary.point_feature_count
              << " lines=" << dataset.geometry.summary.line_feature_count
              << " areas=" << dataset.geometry.summary.area_feature_count << '\n';
  }
  if (!Expect(dataset.geometry.summary.point_feature_count == expected_points,
              std::string(label) + " point feature count mismatch.")) {
    return 1;
  }
  if (!Expect(dataset.geometry.summary.line_feature_count == expected_lines,
              std::string(label) + " line feature count mismatch.")) {
    return 1;
  }
  if (!Expect(dataset.geometry.summary.area_feature_count == expected_areas,
              std::string(label) + " area feature count mismatch.")) {
    return 1;
  }
  if (!Expect(dataset.geometry.summary.vertex_count > 0,
              std::string(label) + " should expose normalized vertices.")) {
    return 1;
  }
  bool has_object_class_info = false;
  for (const auto& feature : dataset.geometry.point_features) {
    if (!feature.object_class_acronym.empty()) {
      has_object_class_info = true;
      break;
    }
  }
  if (!has_object_class_info) {
    for (const auto& feature : dataset.geometry.area_features) {
      if (!feature.object_class_acronym.empty()) {
        has_object_class_info = true;
        break;
      }
    }
  }
  if (!Expect(has_object_class_info,
              std::string(label) + " should resolve S-57 object class metadata.")) {
    return 1;
  }

  const bool has_any_normalized_geometry =
      !dataset.geometry.point_features.empty() ||
      !dataset.geometry.line_features.empty() ||
      !dataset.geometry.area_features.empty();
  if (!Expect(has_any_normalized_geometry,
              std::string(label) + " should expose normalized geometry buckets.")) {
    return 1;
  }

  navscene::render::ChartScene scene;
  navscene::render::AppendDatasetToChartScene(dataset, &scene);
  const auto signature = navscene::render::BuildChartSceneSignature(scene);

  if (scene.stats.point_primitive_count != expected_scene_points ||
      scene.stats.polyline_primitive_count != expected_scene_lines ||
      scene.stats.polygon_primitive_count != expected_scene_polygons) {
    std::cerr << "[navscene-s57-samples] " << label
              << " actual scene primitive counts: points="
              << scene.stats.point_primitive_count
              << " lines=" << scene.stats.polyline_primitive_count
              << " polygons=" << scene.stats.polygon_primitive_count << '\n';
  }
  if (!Expect(scene.stats.point_primitive_count == expected_scene_points,
              std::string(label) + " scene point primitive count mismatch.")) {
    return 1;
  }
  if (!Expect(scene.stats.polyline_primitive_count == expected_scene_lines,
              std::string(label) + " scene polyline primitive count mismatch.")) {
    return 1;
  }
  if (!Expect(scene.stats.polygon_primitive_count == expected_scene_polygons,
              std::string(label) + " scene polygon primitive count mismatch.")) {
    return 1;
  }

  if (!expected_point_class.empty() &&
      !Expect(ContainsObjectClass(scene.points, expected_point_class),
              std::string(label) + " should preserve expected point object class into scene.")) {
    return 1;
  }
  if (!expected_line_class.empty() &&
      !Expect(ContainsObjectClass(scene.polylines, expected_line_class),
              std::string(label) + " should preserve expected line object class into scene.")) {
    return 1;
  }
  if (!expected_area_class.empty() &&
      !Expect(ContainsObjectClass(scene.polygons, expected_area_class),
              std::string(label) + " should preserve expected area object class into scene.")) {
    return 1;
  }
  if (signature.fingerprint64 != expected_scene_fingerprint) {
    std::cerr << "[navscene-s57-samples] " << label
              << " scene signature actual: "
              << navscene::render::FormatChartSceneSignature(signature) << '\n';
  }
  if (!Expect(signature.fingerprint64 == expected_scene_fingerprint,
              std::string(label) + " scene signature fingerprint mismatch.")) {
    return 1;
  }

  const auto svg = navscene::render::ExportChartSceneToSvg(
      scene,
      navscene::render::SvgExportOptions{
          .width = 1024,
          .height = 768,
          .padding = 24,
      });
  const uint64_t svg_fingerprint = Fnv1a64(svg);
  if (std::getenv("NAVSCENE_PRINT_SAMPLE_SVG_HASHES") != nullptr) {
    std::cout << "[navscene-s57-samples] " << label
              << " svg fingerprint=" << svg_fingerprint << '\n';
  }
  if (!Expect(svg.find("<svg") != std::string::npos,
              std::string(label) + " SVG export should contain an svg root.")) {
    return 1;
  }
  if (!expected_area_class.empty() &&
      !Expect(svg.find(std::string("data-obj=\"") + std::string(expected_area_class) + "\"") !=
                  std::string::npos,
              std::string(label) + " SVG should preserve area object class metadata.")) {
    return 1;
  }
  if (expected_svg_fingerprint != 0 &&
      !Expect(svg_fingerprint == expected_svg_fingerprint,
              std::string(label) + " SVG fingerprint mismatch.")) {
    std::cerr << "[navscene-s57-samples] " << label
              << " actual svg fingerprint: " << svg_fingerprint << '\n';
    return 1;
  }

  return 0;
}

int ValidateDirectoryDataset(navscene::IEngine* engine, const fs::path& chart_dir) {
  const auto add_status = engine->Catalog().AddChartDirectory(chart_dir.string());
  if (!Expect(add_status.ok(),
              "AddChartDirectory should succeed for the real US1EEZ3M sample dir.")) {
    return 1;
  }

  const auto rescan_status = engine->Catalog().Rescan();
  if (!Expect(rescan_status.ok(),
              "Rescan should succeed for the real US1EEZ3M sample dir.")) {
    return 1;
  }

  const auto dataset = FindDataset(engine->Catalog().Snapshot(), "US1EEZ3M.000");
  if (!Expect(dataset.has_value(),
              "Catalog snapshot should contain the scanned US1EEZ3M sample.")) {
    return 1;
  }

  if (!Expect(dataset->compilation_scale == 3500000,
              "US1EEZ3M should expose the expected compilation scale.")) {
    return 1;
  }
  if (!ExpectNear(dataset->coverage.min_lon, -174.270659, 0.01,
                  "US1EEZ3M min_lon should match the GDAL sample extent.")) {
    return 1;
  }
  if (!ExpectNear(dataset->coverage.min_lat, -17.807370, 0.01,
                  "US1EEZ3M min_lat should match the GDAL sample extent.")) {
    return 1;
  }
  if (!ExpectNear(dataset->coverage.max_lon, -164.758729, 0.01,
                  "US1EEZ3M max_lon should match the GDAL sample extent.")) {
    return 1;
  }
  if (!ExpectNear(dataset->coverage.max_lat, -9.824997, 0.01,
                  "US1EEZ3M max_lat should match the GDAL sample extent.")) {
    return 1;
  }

  return 0;
}

int ValidateBundledReferenceDefaultView() {
  const auto project_root = FindProjectRoot();
  if (!Expect(project_root.has_value(),
              "Bundled project root should be discoverable for reference-view validation.")) {
    return 1;
  }

  navscene::data::s57::DatasetInfo dataset;
  const fs::path dataset_path = *project_root / "data" / "s57" / "GB4X0000.000";
  const auto load_status = navscene::data::s57::LoadDataset(
      navscene::data::s57::ReadRequest{dataset_path, navscene::SourceType::kChartDataset},
      &dataset);
  if (!Expect(load_status.ok(),
              "Bundled GB4X0000 dataset should load for default-view validation.")) {
    return 1;
  }

  if (!Expect(dataset.descriptor.compilation_scale == 52000,
              "GB4X0000 should expose the expected compilation scale.")) {
    return 1;
  }
  if (!Expect(dataset.descriptor.default_display_scale == 52000,
              "GB4X0000 should expose the expected default display scale hint.")) {
    return 1;
  }
  if (!Expect(dataset.descriptor.has_default_view_center,
              "GB4X0000 should expose a preferred default view center from M_COVR.")) {
    return 1;
  }
  if (!Expect(dataset.descriptor.default_view_center.lon >= 60.94 &&
                  dataset.descriptor.default_view_center.lon <= 61.06,
              "GB4X0000 preferred center longitude should stay within the expected harbor view.")) {
    return 1;
  }
  if (!Expect(dataset.descriptor.default_view_center.lat >= -32.49 &&
                  dataset.descriptor.default_view_center.lat <= -32.43,
              "GB4X0000 preferred center latitude should stay within the expected harbor view.")) {
    return 1;
  }

  const auto viewport =
      navscene::core::MakePreferredViewport(dataset.descriptor, 1280, 1024);
  const auto viewport_coverage =
      navscene::geo::ComputeViewportCoverage(dataset.descriptor.coverage, viewport);
  const double display_scale =
      navscene::geo::EstimateViewportDisplayScale(viewport_coverage, viewport);
  if (!ExpectNear(display_scale, 52000.0, 200.0,
                  "Preferred viewport should target the dataset default display scale.")) {
    return 1;
  }

  return 0;
}

}  // namespace

int main() {
#if !defined(NAVSCENE_HAS_GDAL)
  std::cout << "[navscene-s57-samples] skipped: GDAL is not enabled.\n";
  return 0;
#else
  const fs::path sample_root = ResolveSampleRoot();
  const fs::path direct_dataset = sample_root / "US" / "US2ARCEB" / "US2ARCEB.000";
  const fs::path chart_dir = sample_root / "US" / "US1EEZ3M";

  if (!Expect(fs::exists(direct_dataset),
              "Direct sample dataset is missing. Set NAVSCENE_S57_SAMPLE_ROOT if needed.")) {
    return 1;
  }
  if (!Expect(fs::exists(chart_dir),
              "Chart directory sample is missing. Set NAVSCENE_S57_SAMPLE_ROOT if needed.")) {
    return 1;
  }

  auto engine = navscene::CreateEngine(navscene::EngineConfig{});
  if (!Expect(engine != nullptr, "Engine creation returned null.")) {
    return 1;
  }

  if (const int status = ValidateDirectDataset(engine.get(), direct_dataset); status != 0) {
    return status;
  }
  if (const int status =
          ValidateNormalizedGeometry(
              direct_dataset,
              3,
              0,
              18,
              23,
              0,
              18,
              9923378863221535381ull,
              0ull,
              "SOUNDG",
              "",
              "DEPARE",
              "US2ARCEB");
      status != 0) {
    return status;
  }
  if (const int status = ValidateDirectoryDataset(engine.get(), chart_dir); status != 0) {
    return status;
  }
  if (const int status = ValidateNormalizedGeometry(chart_dir / "US1EEZ3M.000",
                                                    10,
                                                    9,
                                                    49,
                                                    59,
                                                    9,
                                                    49,
                                                    14495648434326235492ull,
                                                    0ull,
                                                    "SOUNDG",
                                                    "COALNE",
                                                    "DEPARE",
                                                    "US1EEZ3M");
      status != 0) {
    return status;
  }
  if (const int status = ValidateBundledReferenceDefaultView(); status != 0) {
    return status;
  }

  return 0;
#endif
}
