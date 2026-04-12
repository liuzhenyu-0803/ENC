#include "navscene/navscene.h"

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

  std::cerr << "[navscene-errors] " << message << '\n';
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

}  // namespace

int main() {
  std::error_code ec;
  const fs::path root = fs::temp_directory_path() / "navscene-errors";
  fs::remove_all(root, ec);
  ec.clear();

  const fs::path empty_dir = root / "empty";
  const fs::path regular_file = root / "not-a-directory.txt";
  const fs::path missing_path = root / "missing";
  fs::create_directories(empty_dir, ec);
  if (!Expect(!ec, "Failed to create empty directory.")) {
    return 1;
  }
  if (!Expect(WriteFile(regular_file, "plain text"),
              "Failed to create regular file sample.")) {
    return 1;
  }

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
              "Default backend should resolve to the expected implementation.")) {
    return 1;
  }

  navscene::EngineConfig fallback_config;
  fallback_config.backend.preferred_backend = navscene::GraphicsBackend::kDirect3D12;
  auto fallback_engine = navscene::CreateEngine(fallback_config);
  if (!Expect(fallback_engine != nullptr,
              "Fallback engine creation should succeed.")) {
    return 1;
  }
  if (!Expect(fallback_engine->RenderSurface().GetActiveBackend() ==
                  navscene::GraphicsBackend::kSoftware,
              "Unsupported preferred backend should fall back to Software.")) {
    return 1;
  }

  navscene::EngineConfig strict_config;
  strict_config.backend.preferred_backend = navscene::GraphicsBackend::kDirect3D12;
  strict_config.backend.allow_fallback = false;
  auto strict_engine = navscene::CreateEngine(strict_config);
  if (!Expect(strict_engine != nullptr, "Strict engine creation should succeed.")) {
    return 1;
  }
  if (!Expect(strict_engine->RenderSurface().GetActiveBackend() ==
                  navscene::GraphicsBackend::kNull,
              "Unsupported preferred backend should resolve to Null when fallback is disabled.")) {
    return 1;
  }

  const auto empty_fit_status = engine->MapView().FitToData();
  if (!Expect(!empty_fit_status.ok() &&
                  empty_fit_status.code == navscene::StatusCode::kNotFound,
              "FitToData should report missing coverage when no datasets are loaded.")) {
    return 1;
  }

  if (!Expect(!engine->Catalog().AddChartDirectory("").ok(),
              "Empty chart directory path should fail.")) {
    return 1;
  }
  if (!Expect(!engine->Catalog().AddChartDirectory(missing_path.string()).ok(),
              "Missing chart directory path should fail.")) {
    return 1;
  }
  if (!Expect(!engine->Catalog().AddChartDirectory(regular_file.string()).ok(),
              "Regular file should not be accepted as chart directory.")) {
    return 1;
  }
  if (!Expect(engine->Catalog().AddChartDirectory(empty_dir.string()).ok(),
              "Empty directory registration should still succeed.")) {
    return 1;
  }
  if (!Expect(!engine->Catalog().Rescan().ok(),
              "Rescan should report no datasets for an empty directory.")) {
    return 1;
  }
  if (!Expect(engine->Catalog().Snapshot().empty(),
              "Empty directory rescan should not add datasets.")) {
    return 1;
  }
  if (!Expect(!engine->Catalog().RemoveChartDirectory("not-registered").ok(),
              "Removing an unregistered chart directory should fail.")) {
    return 1;
  }

  if (!Expect(!engine->RenderSurface().RenderFrame().ok(),
              "RenderFrame should fail before a surface is attached.")) {
    return 1;
  }
  if (!Expect(!engine->RenderSurface().Resize(640, 480).ok(),
              "Resize should fail before a surface is attached.")) {
    return 1;
  }

  navscene::NativeSurfaceDesc invalid_window_surface;
  invalid_window_surface.type = navscene::SurfaceType::kWindow;
  invalid_window_surface.platform = navscene::NativePlatform::kWin32;
  invalid_window_surface.width = 640;
  invalid_window_surface.height = 480;
  if (!Expect(!engine->RenderSurface().AttachSurface(invalid_window_surface).ok(),
              "Window surface attach should fail when the native handle is null.")) {
    return 1;
  }

  navscene::SourceDescriptor source;
  source.id = "invalid-source";
  source.uri = regular_file.string();
  source.type = navscene::SourceType::kChartDataset;
  source.format = navscene::DatasetFormat::kS57;

  const auto register_status = engine->Sources().RegisterSource(source);
  if (!Expect(!register_status.ok(),
              "A non-.000 regular file should not be accepted as an ENC source.")) {
    return 1;
  }
  if (!Expect(engine->Sources().Snapshot().empty(),
              "A failed source registration must not leave stale sources behind.")) {
    return 1;
  }
  if (!Expect(engine->Catalog().Snapshot().empty(),
              "A failed source registration must not change the catalog snapshot.")) {
    return 1;
  }

  fs::remove_all(root, ec);
  return 0;
}
