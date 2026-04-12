#include "render/backend.h"

#include "render/vulkan_backend.h"
#include "render/win32_gdi_presenter.h"

namespace navscene::render {
namespace {

Status OkStatus() { return {}; }

Status ErrorStatus(StatusCode code, std::string message) {
  return Status{code, std::move(message)};
}

class NullRendererBackend final : public IRendererBackend {
 public:
  GraphicsBackend backend_type() const override { return GraphicsBackend::kNull; }
  RenderMode last_render_mode() const override { return RenderMode::kNull; }

  Status AttachSurface(const NativeSurfaceDesc&) override { return OkStatus(); }
  Status DetachSurface() override { return OkStatus(); }
  Status Resize(const NativeSurfaceDesc&) override { return OkStatus(); }

  Status RenderFrame(const portrayal::PortrayalScene&,
                     const GeoBox&,
                     const Viewport&,
                     const NativeSurfaceDesc&) override {
    return OkStatus();
  }
};

class SoftwareRendererBackend final : public IRendererBackend {
 public:
  GraphicsBackend backend_type() const override { return GraphicsBackend::kSoftware; }
  RenderMode last_render_mode() const override { return RenderMode::kSoftwareRaster; }

  Status AttachSurface(const NativeSurfaceDesc& surface) override {
    return ValidateSurface(surface);
  }

  Status DetachSurface() override { return OkStatus(); }

  Status Resize(const NativeSurfaceDesc& surface) override {
    return ValidateSurface(surface);
  }

  Status RenderFrame(const portrayal::PortrayalScene& scene,
                     const GeoBox& coverage,
                     const Viewport& viewport,
                     const NativeSurfaceDesc& surface) override {
    const auto validation = ValidateSurface(surface);
    if (!validation.ok()) {
      return validation;
    }

    if (surface.type == SurfaceType::kOffscreen) {
      return OkStatus();
    }

    return PresentChartSceneWin32Gdi(scene, coverage, viewport, surface);
  }

 private:
  Status ValidateSurface(const NativeSurfaceDesc& surface) const {
    if (surface.type == SurfaceType::kOffscreen) {
      return OkStatus();
    }

    if (surface.type != SurfaceType::kWindow) {
      return ErrorStatus(StatusCode::kUnsupported,
                         "Software renderer backend only supports window or offscreen surfaces.");
    }
    if (surface.window_handle == nullptr) {
      return ErrorStatus(StatusCode::kInvalidArgument,
                         "Native window handle must not be null.");
    }
#if defined(_WIN32)
    if (surface.platform != NativePlatform::kWin32) {
      return ErrorStatus(StatusCode::kUnsupported,
                         "Software renderer backend currently supports only Win32 window surfaces.");
    }
    return OkStatus();
#else
    return ErrorStatus(StatusCode::kUnsupported,
                       "Software renderer backend window presentation is only implemented on Windows.");
#endif
  }
};

}  // namespace

std::unique_ptr<IRendererBackend> CreateRendererBackend(GraphicsBackend backend) {
  switch (backend) {
    case GraphicsBackend::kVulkan:
      if (auto vulkan = CreateVulkanRendererBackend()) {
        return vulkan;
      }
      return std::make_unique<SoftwareRendererBackend>();
    case GraphicsBackend::kSoftware:
      return std::make_unique<SoftwareRendererBackend>();
    case GraphicsBackend::kNull:
      return std::make_unique<NullRendererBackend>();
    default:
      return std::make_unique<NullRendererBackend>();
  }
}

}  // namespace navscene::render
