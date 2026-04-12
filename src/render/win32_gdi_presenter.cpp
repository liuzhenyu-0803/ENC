#include "render/win32_gdi_presenter.h"

#include "portrayal/engine.h"
#include "render/software_raster.h"

#if defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace navscene::render {

namespace {

portrayal::DisplaySettings BuildCompatibilitySettings() {
  auto settings = portrayal::MakeDisplaySettings(DisplayOptions{});
  settings.display_category = DisplayCategory::kAll;
  settings.show_meta = true;
  settings.show_quality_of_data = true;
  return settings;
}

}  // namespace

Status PresentChartSceneWin32Gdi(const ChartScene& scene,
                                 const GeoBox& reference_coverage,
                                 const Viewport& viewport,
                                 const NativeSurfaceDesc& surface) {
  return PresentChartSceneWin32Gdi(
      portrayal::BuildPortrayalScene(scene, BuildCompatibilitySettings()),
      reference_coverage,
      viewport,
      surface);
}

Status PresentChartSceneWin32Gdi(const portrayal::PortrayalScene& scene,
                                 const GeoBox& reference_coverage,
                                 const Viewport& viewport,
                                 const NativeSurfaceDesc& surface) {
  if (surface.type != SurfaceType::kWindow || surface.window_handle == nullptr) {
    return Status{StatusCode::kInvalidArgument,
                  "Win32 GDI presenter requires a native window handle."};
  }
  if (surface.platform != NativePlatform::kWin32) {
    return Status{StatusCode::kUnsupported,
                  "Win32 GDI presenter only supports Win32 surfaces."};
  }

  HWND hwnd = static_cast<HWND>(surface.window_handle);
  RECT rect{};
  if (!GetClientRect(hwnd, &rect)) {
    return Status{StatusCode::kIoError, "GetClientRect failed for the render surface."};
  }

  const int width = rect.right - rect.left;
  const int height = rect.bottom - rect.top;
  if (width <= 0 || height <= 0) {
    return {};
  }

  SoftwareRasterImage image;
  const auto raster_status = RasterizeChartSceneWin32(scene,
                                                      reference_coverage,
                                                      viewport,
                                                      static_cast<uint32_t>(width),
                                                      static_cast<uint32_t>(height),
                                                      &image);
  if (!raster_status.ok()) {
    return raster_status;
  }

  return PresentSoftwareRasterWin32(image, surface);
}

}  // namespace navscene::render

#else

namespace navscene::render {

Status PresentChartSceneWin32Gdi(const ChartScene&,
                                 const GeoBox&,
                                 const Viewport&,
                                 const NativeSurfaceDesc&) {
  return Status{StatusCode::kUnsupported,
                "Win32 GDI presenter is only available on Windows."};
}

Status PresentChartSceneWin32Gdi(const portrayal::PortrayalScene&,
                                 const GeoBox&,
                                 const Viewport&,
                                 const NativeSurfaceDesc&) {
  return Status{StatusCode::kUnsupported,
                "Win32 GDI presenter is only available on Windows."};
}

}  // namespace navscene::render

#endif
