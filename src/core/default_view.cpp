#include "core/default_view.h"

#include "geo/mercator_projection.h"

namespace navscene::core {

Viewport MakePreferredViewport(const DatasetDescriptor& descriptor,
                               uint32_t width,
                               uint32_t height,
                               int padding_pixels) {
  Viewport viewport =
      geo::MakeFittedViewport(descriptor.coverage, width, height, 1.0, 0.0, padding_pixels);
  if (!geo::HasValidCoverage(descriptor.coverage)) {
    return viewport;
  }

  if (descriptor.default_display_scale > 0) {
    const GeoBox fitted_coverage =
        geo::ComputeViewportCoverage(descriptor.coverage, viewport, padding_pixels);
    const double fitted_display_scale =
        geo::EstimateViewportDisplayScale(fitted_coverage, viewport, padding_pixels);
    if (fitted_display_scale > 0.0) {
      viewport.scale_ppm =
          fitted_display_scale / static_cast<double>(descriptor.default_display_scale);
    }
  }

  if (descriptor.has_default_view_center) {
    viewport.center = descriptor.default_view_center;
  }

  return viewport;
}

}  // namespace navscene::core
