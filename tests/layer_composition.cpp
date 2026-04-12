#include "navscene/navscene.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <system_error>

namespace fs = std::filesystem;

namespace {

bool Expect(bool condition, std::string_view message) {
  if (condition) {
    return true;
  }

  std::cerr << "[navscene-layer-composition] " << message << '\n';
  return false;
}

bool WriteFile(const fs::path& path, std::string_view content) {
  std::ofstream stream(path, std::ios::binary);
  if (!stream.is_open()) {
    return false;
  }

  stream << content;
  return stream.good();
}

fs::path ResolveSampleRoot() {
  if (const char* env = std::getenv("NAVSCENE_S57_SAMPLE_ROOT")) {
    if (*env != '\0') {
      return fs::path(env);
    }
  }
  return fs::path("E:/projects/enc/enc");
}

bool RenderAndExpectVisibleCount(navscene::IEngine* engine, uint64_t expected_visible_count) {
  if (!Expect(engine != nullptr, "Engine should not be null.")) {
    return false;
  }

  const auto render_status = engine->RenderSurface().RenderFrame();
  if (!Expect(render_status.ok(), "RenderFrame should succeed.")) {
    return false;
  }

  const auto stats = engine->Diagnostics().GetLastFrameStats();
  if (!Expect(stats.visible_datasets == expected_visible_count,
              "FrameStats.visible_datasets should match the expected layer-filtered count.")) {
    return false;
  }
  if (!Expect(engine->GetVisibleDatasets().size() == expected_visible_count,
              "GetVisibleDatasets should match the expected layer-filtered count.")) {
    return false;
  }

  return true;
}

}  // namespace

int main() {
#if defined(NAVSCENE_HAS_GDAL)
  const fs::path sample_root = ResolveSampleRoot();
  const fs::path chart_dir = sample_root / "US" / "US2ARCEB";
  const fs::path direct_dataset = sample_root / "US" / "US1EEZ3M" / "US1EEZ3M.000";

  if (!Expect(fs::exists(chart_dir),
              "Layer composition test requires a real chart directory sample.")) {
    return 1;
  }
  if (!Expect(fs::exists(direct_dataset),
              "Layer composition test requires a real direct dataset sample.")) {
    return 1;
  }
#else
  std::error_code ec;
  const fs::path root = fs::temp_directory_path() / "navscene-layer-composition";
  fs::remove_all(root, ec);
  ec.clear();

  const fs::path chart_dir = root / "charts";
  const fs::path direct_dir = root / "direct";
  const fs::path direct_dataset = direct_dir / "CELL_B.000";
  fs::create_directories(chart_dir, ec);
  fs::create_directories(direct_dir, ec);
  if (!Expect(!ec, "Failed to create temporary chart directories.")) {
    return 1;
  }
  if (!Expect(WriteFile(chart_dir / "CELL_A.000", "dummy enc"),
              "Failed to write directory chart sample.")) {
    return 1;
  }
  if (!Expect(WriteFile(direct_dataset, "dummy enc"),
              "Failed to write direct chart sample.")) {
    return 1;
  }
#endif

  auto engine = navscene::CreateEngine(navscene::EngineConfig{});
  if (!Expect(engine != nullptr, "Engine creation returned null.")) {
    return 1;
  }

  if (!Expect(engine->Catalog().AddChartDirectory(chart_dir.string()).ok(),
              "AddChartDirectory should succeed.")) {
    return 1;
  }
  if (!Expect(engine->Catalog().Rescan().ok(),
              "Catalog Rescan should recognize chart datasets.")) {
    return 1;
  }

  navscene::SourceDescriptor direct_source;
  direct_source.id = "direct-cell";
  direct_source.uri = direct_dataset.string();
  direct_source.type = navscene::SourceType::kChartDataset;
  direct_source.format = navscene::DatasetFormat::kS57;
  if (!Expect(engine->Sources().RegisterSource(direct_source).ok(),
              "Direct chart source registration should succeed.")) {
    return 1;
  }

  navscene::NativeSurfaceDesc offscreen_surface;
  offscreen_surface.type = navscene::SurfaceType::kOffscreen;
  offscreen_surface.width = 256;
  offscreen_surface.height = 256;
  if (!Expect(engine->RenderSurface().AttachSurface(offscreen_surface).ok(),
              "Offscreen surface attach should succeed.")) {
    return 1;
  }

  if (!RenderAndExpectVisibleCount(engine.get(), 2)) {
    return 1;
  }

  navscene::LayerDescriptor chart_layer;
  chart_layer.id = "chart-main";
  chart_layer.type = navscene::LayerType::kChart;
  chart_layer.visible = true;
  chart_layer.z_order = 10;
  if (!Expect(engine->Layers().CreateLayer(chart_layer).ok(),
              "Chart layer creation should succeed.")) {
    return 1;
  }

  if (!RenderAndExpectVisibleCount(engine.get(), 1)) {
    return 1;
  }

  navscene::LayerDescriptor source_layer;
  source_layer.id = "chart-direct";
  source_layer.type = navscene::LayerType::kOverlay;
  source_layer.visible = true;
  source_layer.z_order = 20;
  if (!Expect(engine->Layers().CreateLayer(source_layer).ok(),
              "Direct-source layer creation should succeed.")) {
    return 1;
  }
  if (!Expect(engine->Layers().AttachSource(source_layer.id, direct_source.id).ok(),
              "Attaching a direct source to a layer should succeed.")) {
    return 1;
  }
  if (!Expect(engine->Layers().AttachSource(source_layer.id, direct_source.id).ok(),
              "Re-attaching the same source should be idempotent.")) {
    return 1;
  }

  if (!RenderAndExpectVisibleCount(engine.get(), 2)) {
    return 1;
  }

  if (!Expect(engine->Layers().SetLayerVisible(chart_layer.id, false).ok(),
              "Hiding the catalog chart layer should succeed.")) {
    return 1;
  }
  if (!RenderAndExpectVisibleCount(engine.get(), 1)) {
    return 1;
  }

  if (!Expect(engine->Layers().SetLayerVisible(source_layer.id, false).ok(),
              "Hiding the direct-source layer should succeed.")) {
    return 1;
  }
  if (!RenderAndExpectVisibleCount(engine.get(), 0)) {
    return 1;
  }

  const auto hidden_fit_status = engine->MapView().FitToData();
  if (!Expect(!hidden_fit_status.ok() &&
                  hidden_fit_status.code == navscene::StatusCode::kNotFound,
              "FitToData should fail when all layers are hidden.")) {
    return 1;
  }

  if (!Expect(engine->Layers().SetLayerVisible(chart_layer.id, true).ok(),
              "Showing the catalog chart layer again should succeed.")) {
    return 1;
  }
  if (!RenderAndExpectVisibleCount(engine.get(), 1)) {
    return 1;
  }

#if !defined(NAVSCENE_HAS_GDAL)
  fs::remove_all(root, ec);
#endif
  return 0;
}
