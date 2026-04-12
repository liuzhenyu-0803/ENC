#include "render/backend.h"
#include "portrayal/engine.h"
#include "render/scene.h"

#if defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <iostream>
#include <memory>
#include <string>
#include <string_view>

namespace {

bool Expect(bool condition, std::string_view message) {
  if (condition) {
    return true;
  }

  std::cerr << "[navscene-vulkan-win32-render-modes] " << message << '\n';
  return false;
}

std::wstring BuildWindowClassName() {
  return L"navscene_test_window_" +
         std::to_wstring(static_cast<unsigned long>(::GetCurrentProcessId()));
}

LRESULT CALLBACK TestWindowProc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param) {
  return DefWindowProcW(hwnd, message, w_param, l_param);
}

class ScopedWindowClass {
 public:
  ScopedWindowClass() = default;
  ~ScopedWindowClass() {
    if (registered_) {
      UnregisterClassW(class_name_.c_str(), instance_);
    }
  }

  bool Register() {
    instance_ = GetModuleHandleW(nullptr);
    class_name_ = BuildWindowClassName();

    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = TestWindowProc;
    window_class.hInstance = instance_;
    window_class.lpszClassName = class_name_.c_str();

    const ATOM atom = RegisterClassExW(&window_class);
    if (atom == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
      return false;
    }

    registered_ = true;
    return true;
  }

  const wchar_t* class_name() const { return class_name_.c_str(); }
  HINSTANCE instance() const { return instance_; }

 private:
  bool registered_ = false;
  std::wstring class_name_;
  HINSTANCE instance_ = nullptr;
};

class ScopedWindow {
 public:
  ScopedWindow() = default;
  ~ScopedWindow() {
    if (hwnd_ != nullptr) {
      DestroyWindow(hwnd_);
    }
  }

  bool Create(const ScopedWindowClass& window_class, int width, int height) {
    hwnd_ = CreateWindowExW(0,
                            window_class.class_name(),
                            L"navscene Vulkan render mode test",
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            width,
                            height,
                            nullptr,
                            nullptr,
                            window_class.instance(),
                            nullptr);
    if (hwnd_ == nullptr) {
      return false;
    }

    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    UpdateWindow(hwnd_);
    PumpMessages();
    return true;
  }

  HWND hwnd() const { return hwnd_; }

  void PumpMessages() const {
    MSG message{};
    while (PeekMessageW(&message, hwnd_, 0, 0, PM_REMOVE)) {
      TranslateMessage(&message);
      DispatchMessageW(&message);
    }
  }

 private:
  HWND hwnd_ = nullptr;
};

navscene::render::ChartScene BuildSimpleScene() {
  navscene::render::ChartScene scene;

  scene.polygons.push_back(navscene::render::PolygonPrimitive{
      .dataset_path = "synthetic",
      .object_class_code = 42,
      .object_class_acronym = "DEPARE",
      .outer_ring = {
          {.lat = 12.0, .lon = 12.0},
          {.lat = 12.0, .lon = 18.0},
          {.lat = 18.0, .lon = 18.0},
          {.lat = 18.0, .lon = 12.0},
      },
      .holes = {{
          {.lat = 14.0, .lon = 14.0},
          {.lat = 14.0, .lon = 15.0},
          {.lat = 15.0, .lon = 15.0},
      }},
  });

  scene.polylines.push_back(navscene::render::PolylinePrimitive{
      .dataset_path = "synthetic",
      .object_class_code = 30,
      .object_class_acronym = "COALNE",
      .vertices = {
          {.lat = 11.0, .lon = 11.0},
          {.lat = 16.0, .lon = 16.0},
          {.lat = 19.0, .lon = 16.0},
      },
  });

  scene.points.push_back(navscene::render::PointPrimitive{
      .dataset_path = "synthetic",
      .object_class_code = 129,
      .object_class_acronym = "SOUNDG",
      .position = {.lat = 15.0, .lon = 15.0},
  });

  scene.stats.point_primitive_count = static_cast<uint64_t>(scene.points.size());
  scene.stats.polyline_primitive_count = static_cast<uint64_t>(scene.polylines.size());
  scene.stats.polygon_primitive_count = static_cast<uint64_t>(scene.polygons.size());
  return scene;
}

navscene::render::ChartScene BuildDashedScene() {
  navscene::render::ChartScene scene;

  scene.polylines.push_back(navscene::render::PolylinePrimitive{
      .dataset_path = "synthetic",
      .object_class_code = 931,
      .object_class_acronym = "RECTRC",
      .vertices = {
          {.lat = 11.0, .lon = 11.0},
          {.lat = 16.0, .lon = 16.0},
          {.lat = 19.0, .lon = 16.0},
      },
  });

  scene.stats.polyline_primitive_count = static_cast<uint64_t>(scene.polylines.size());
  return scene;
}

navscene::GeoBox BuildCoverage() {
  return navscene::GeoBox{
      .min_lat = 10.0,
      .min_lon = 10.0,
      .max_lat = 20.0,
      .max_lon = 20.0,
  };
}

navscene::Viewport BuildViewport() {
  return navscene::Viewport{
      .center = {.lat = 15.0, .lon = 15.0},
      .scale_ppm = 1.0,
      .rotation_rad = 0.0,
      .width = 640,
      .height = 480,
  };
}

navscene::NativeSurfaceDesc BuildSurfaceDesc(HWND hwnd) {
  return navscene::NativeSurfaceDesc{
      .type = navscene::SurfaceType::kWindow,
      .platform = navscene::NativePlatform::kWin32,
      .window_handle = hwnd,
      .width = 640,
      .height = 480,
      .dpi_scale = 1.0f,
  };
}

bool VerifyRenderMode(navscene::render::IRendererBackend* backend,
                      HWND hwnd,
                      const navscene::render::ChartScene& scene,
                      const navscene::GeoBox& coverage,
                      const navscene::Viewport& viewport,
                      navscene::RenderMode expected_mode,
                      std::string_view failure_prefix) {
  if (backend == nullptr) {
    return Expect(false, "Renderer backend should not be null.");
  }

  const auto portrayal_scene = navscene::portrayal::BuildPortrayalScene(
      scene,
      navscene::portrayal::MakeDisplaySettings(navscene::DisplayOptions{}));
  const auto surface = BuildSurfaceDesc(hwnd);
  const auto attach_status = backend->AttachSurface(surface);
  if (!Expect(attach_status.ok(), std::string(failure_prefix) + ": attach should succeed.")) {
    return false;
  }

  const auto render_status =
      backend->RenderFrame(portrayal_scene, coverage, viewport, surface);
  if (!Expect(render_status.ok(), std::string(failure_prefix) + ": render should succeed.")) {
    backend->DetachSurface();
    return false;
  }

  const bool mode_matches = Expect(
      backend->last_render_mode() == expected_mode,
      std::string(failure_prefix) + ": backend should report the expected render mode.");
  const auto detach_status = backend->DetachSurface();
  if (!Expect(detach_status.ok(), std::string(failure_prefix) + ": detach should succeed.")) {
    return false;
  }

  return mode_matches;
}

}  // namespace

int main() {
#if !defined(NAVSCENE_HAS_VULKAN)
  std::cerr << "[navscene-vulkan-win32-render-modes] Vulkan backend is not enabled.\n";
  return 1;
#else
  ScopedWindowClass window_class;
  if (!Expect(window_class.Register(), "Win32 test window class registration should succeed.")) {
    return 1;
  }

  ScopedWindow window;
  if (!Expect(window.Create(window_class, 640, 480),
              "Win32 test window creation should succeed.")) {
    return 1;
  }

  auto backend = navscene::render::CreateRendererBackend(navscene::GraphicsBackend::kVulkan);
  if (!Expect(backend != nullptr, "Vulkan renderer backend should be created.")) {
    return 1;
  }
  if (!Expect(backend->backend_type() == navscene::GraphicsBackend::kVulkan,
              "Created renderer backend should be Vulkan.")) {
    return 1;
  }

  if (!VerifyRenderMode(backend.get(),
                        window.hwnd(),
                        BuildSimpleScene(),
                        BuildCoverage(),
                        BuildViewport(),
                        navscene::RenderMode::kGpuNativeGeometry,
                        "native render path")) {
    return 1;
  }

  if (!VerifyRenderMode(backend.get(),
                        window.hwnd(),
                        BuildSimpleScene(),
                        navscene::GeoBox{},
                        BuildViewport(),
                        navscene::RenderMode::kGpuRasterUpload,
                        "fallback render path")) {
    return 1;
  }

  if (!VerifyRenderMode(backend.get(),
                        window.hwnd(),
                        BuildDashedScene(),
                        BuildCoverage(),
                        BuildViewport(),
                        navscene::RenderMode::kGpuRasterUpload,
                        "dashed-line fallback path")) {
    return 1;
  }

  return 0;
#endif
}

#else

int main() { return 0; }

#endif
