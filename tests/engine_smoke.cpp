#include "navscene/navscene.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <string_view>
#include <system_error>

namespace fs = std::filesystem;

namespace {

bool Expect(bool condition, std::string_view message) {
  if (condition) {
    return true;
  }

  std::cerr << "[navscene-smoke] " << message << '\n';
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

}  // namespace

int main() {
#if defined(NAVSCENE_HAS_GDAL)
  const fs::path sample_root = ResolveSampleRoot();
  const fs::path chart_dir = sample_root / "US" / "US2ARCEB";
  const fs::path direct_dataset = sample_root / "US" / "US1EEZ3M" / "US1EEZ3M.000";

  if (!Expect(fs::exists(chart_dir),
              "GDAL smoke test requires a real chart directory sample.")) {
    return 1;
  }
  if (!Expect(fs::exists(direct_dataset),
              "GDAL smoke test requires a real direct dataset sample.")) {
    return 1;
  }
#else
  std::error_code ec;
  const fs::path root = fs::temp_directory_path() / "navscene-smoke";
  fs::remove_all(root, ec);
  ec.clear();

  const fs::path chart_dir = root / "charts";
  const fs::path extra_dir = root / "extra";
  fs::create_directories(chart_dir, ec);
  if (!Expect(!ec, "Failed to create chart directory.")) {
    return 1;
  }

  fs::create_directories(extra_dir, ec);
  if (!Expect(!ec, "Failed to create extra directory.")) {
    return 1;
  }

  if (!Expect(WriteFile(chart_dir / "CELL_A.000", "dummy enc"),
              "Failed to write chart sample.")) {
    return 1;
  }
  if (!Expect(WriteFile(chart_dir / "README.txt", "ignore"),
              "Failed to write ignored sample.")) {
    return 1;
  }
  if (!Expect(WriteFile(extra_dir / "CELL_B.000", "dummy enc"),
              "Failed to write direct source sample.")) {
    return 1;
  }
#endif

  auto engine = navscene::CreateEngine(navscene::EngineConfig{});
  if (!Expect(engine != nullptr, "Engine creation returned null.")) {
    return 1;
  }
#if defined(NAVSCENE_HAS_VULKAN)
  constexpr auto kExpectedDefaultBackend = navscene::GraphicsBackend::kVulkan;
#else
  constexpr auto kExpectedDefaultBackend = navscene::GraphicsBackend::kSoftware;
#endif
  if (!Expect(engine->RenderSurface().GetActiveBackend() ==
                  kExpectedDefaultBackend,
              "Default engine backend should resolve to the expected implementation.")) {
    return 1;
  }

  const auto add_status = engine->Catalog().AddChartDirectory(chart_dir.string());
  if (!Expect(add_status.ok(), "AddChartDirectory should succeed.")) {
    return 1;
  }

  const auto rescan_status = engine->Catalog().Rescan();
  if (!Expect(rescan_status.ok(), "Catalog Rescan should recognize .000 files.")) {
    return 1;
  }

  const auto scanned_datasets = engine->Catalog().Snapshot();
  if (!Expect(scanned_datasets.size() == 1,
              "Catalog snapshot should contain one scanned dataset.")) {
    return 1;
  }
  if (!Expect(scanned_datasets.front().format == navscene::DatasetFormat::kS57,
              "Scanned dataset should be marked as S-57.")) {
    return 1;
  }

  navscene::SourceDescriptor direct_source;
  direct_source.id = "direct-cell";
#if defined(NAVSCENE_HAS_GDAL)
  direct_source.uri = direct_dataset.string();
#else
  direct_source.uri = (extra_dir / "CELL_B.000").string();
#endif
  direct_source.type = navscene::SourceType::kChartDataset;
  direct_source.format = navscene::DatasetFormat::kS57;

  const auto register_status = engine->Sources().RegisterSource(direct_source);
  if (!Expect(register_status.ok(), "RegisterSource should accept a .000 file.")) {
    return 1;
  }

  const auto sources = engine->Sources().Snapshot();
  if (!Expect(sources.size() == 1, "Source snapshot should contain one source.")) {
    return 1;
  }

  const auto merged_datasets = engine->Catalog().Snapshot();
  if (!Expect(merged_datasets.size() == 2,
              "Catalog snapshot should merge scanned and direct datasets.")) {
    return 1;
  }

  navscene::NativeSurfaceDesc offscreen_surface;
  offscreen_surface.type = navscene::SurfaceType::kOffscreen;
  offscreen_surface.width = 256;
  offscreen_surface.height = 256;

  const auto attach_status = engine->RenderSurface().AttachSurface(offscreen_surface);
  if (!Expect(attach_status.ok(), "Offscreen surface attach should succeed.")) {
    return 1;
  }

  const auto frame_status = engine->RenderSurface().RenderFrame();
  if (!Expect(frame_status.ok(), "RenderFrame should succeed after attach.")) {
    return 1;
  }

  const auto frame_stats = engine->Diagnostics().GetLastFrameStats();
  if (!Expect(frame_stats.visible_datasets == 2,
              "Frame stats should report the recognized dataset count.")) {
    return 1;
  }

#if defined(NAVSCENE_HAS_GDAL)
  if (!Expect(frame_stats.rendered_primitives > 0,
              "GDAL smoke test should produce chart scene primitives.")) {
    return 1;
  }

  const auto initial_viewport = engine->MapView().GetViewport();
  if (!Expect(initial_viewport.width == 256 && initial_viewport.height == 256,
              "Viewport should inherit the attached surface size.")) {
    return 1;
  }
  if (!Expect(initial_viewport.scale_ppm > 0.0,
              "Viewport should be initialized with a positive zoom factor.")) {
    return 1;
  }

  const auto fit_status = engine->MapView().FitToData();
  if (!Expect(fit_status.ok(), "FitToData should succeed once chart coverage is loaded.")) {
    return 1;
  }
  const auto fitted_viewport = engine->MapView().GetViewport();
  if (!Expect(fitted_viewport.width == initial_viewport.width &&
                  fitted_viewport.height == initial_viewport.height,
              "FitToData should preserve the current surface extent.")) {
    return 1;
  }

  const auto pan_status = engine->Input().PanPixels(12.0, -8.0);
  if (!Expect(pan_status.ok(), "PanPixels should succeed once GDAL data is loaded.")) {
    return 1;
  }
  const auto panned_viewport = engine->MapView().GetViewport();
  if (!Expect(panned_viewport.center.lon != initial_viewport.center.lon ||
                  panned_viewport.center.lat != initial_viewport.center.lat,
              "PanPixels should change the viewport center.")) {
    return 1;
  }

  const auto zoom_status = engine->Input().ZoomAt(128.0, 128.0, 1.0);
  if (!Expect(zoom_status.ok(), "ZoomAt should succeed once viewport is ready.")) {
    return 1;
  }
  const auto zoomed_viewport = engine->MapView().GetViewport();
  if (!Expect(zoomed_viewport.scale_ppm > panned_viewport.scale_ppm,
              "ZoomAt should increase the viewport zoom factor.")) {
    return 1;
  }

  const auto refit_status = engine->MapView().FitToData();
  if (!Expect(refit_status.ok(), "FitToData should also recover a user-adjusted viewport.")) {
    return 1;
  }
  const auto refitted_viewport = engine->MapView().GetViewport();
  if (!Expect(refitted_viewport.center.lon != panned_viewport.center.lon ||
                  refitted_viewport.center.lat != panned_viewport.center.lat ||
                  refitted_viewport.scale_ppm != zoomed_viewport.scale_ppm,
              "FitToData should restore the coverage-driven viewport after pan/zoom.")) {
    return 1;
  }

  const auto resize_status = engine->RenderSurface().Resize(512, 384);
  if (!Expect(resize_status.ok(), "Resize should succeed after surface attach.")) {
    return 1;
  }
  const auto resized_viewport = engine->MapView().GetViewport();
  if (!Expect(resized_viewport.width == 512 && resized_viewport.height == 384,
              "Resize should update the viewport extent.")) {
    return 1;
  }

  const auto redraw_status = engine->RenderSurface().RenderFrame();
  if (!Expect(redraw_status.ok(), "RenderFrame should still succeed after resize.")) {
    return 1;
  }
#endif

#if !defined(NAVSCENE_HAS_GDAL)
  fs::remove_all(root, ec);
#endif
  return 0;
}
