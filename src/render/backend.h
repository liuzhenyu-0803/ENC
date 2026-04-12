#pragma once

#include "navscene/navscene.h"
#include "portrayal/scene.h"
#include "render/scene.h"

#include <memory>

namespace navscene::render {

class IRendererBackend {
 public:
  virtual ~IRendererBackend() = default;

  virtual GraphicsBackend backend_type() const = 0;
  virtual RenderMode last_render_mode() const = 0;
  virtual Status AttachSurface(const NativeSurfaceDesc& surface) = 0;
  virtual Status DetachSurface() = 0;
  virtual Status Resize(const NativeSurfaceDesc& surface) = 0;
  virtual Status RenderFrame(const portrayal::PortrayalScene& scene,
                             const GeoBox& coverage,
                             const Viewport& viewport,
                             const NativeSurfaceDesc& surface) = 0;
};

std::unique_ptr<IRendererBackend> CreateRendererBackend(GraphicsBackend backend);

}  // namespace navscene::render
