#pragma once

#include "navscene/navscene.h"

#include <cstdint>
#include <utility>

namespace navscene::geo {

struct ViewportProjection {
  bool valid = false;
  double center_x_px = 0.0;
  double center_y_px = 0.0;
  double draw_width_px = 0.0;
  double draw_height_px = 0.0;
  double pixels_per_meter = 0.0;
};

bool HasValidCoverage(const GeoBox& coverage);

double NormalizeLongitudeDegrees(double lon, double reference_lon);

double MercatorNorthingMeters(double lat_deg);

double InverseMercatorLatitudeDegrees(double northing_m);

double MercatorEastingDeltaMeters(double lon_deg, double reference_lon_deg);

double ComputeCoverageWidthMeters(const GeoBox& coverage);

double ComputeCoverageHeightMeters(const GeoBox& coverage);

double ComputeFitPixelsPerMeter(const GeoBox& coverage,
                                uint32_t width,
                                uint32_t height,
                                int padding_pixels = 0);

Viewport MakeFittedViewport(const GeoBox& coverage,
                            uint32_t width,
                            uint32_t height,
                            double scale_ppm = 1.0,
                            double rotation_rad = 0.0,
                            int padding_pixels = 0);

ViewportProjection BuildViewportProjection(const GeoBox& coverage,
                                           const Viewport& viewport,
                                           int padding_pixels = 0);

std::pair<double, double> GeoToPixel(const GeoBox& coverage,
                                     const Viewport& viewport,
                                     const GeoPoint& point,
                                     int padding_pixels = 0);

GeoPoint PixelToGeo(const GeoBox& coverage,
                    const Viewport& viewport,
                    double pixel_x,
                    double pixel_y,
                    int padding_pixels = 0);

GeoBox ComputeViewportCoverage(const GeoBox& reference_coverage,
                               const Viewport& viewport,
                               int padding_pixels = 0);

double EstimateViewportDisplayScale(const GeoBox& viewport_coverage,
                                    const Viewport& viewport,
                                    int padding_pixels = 0);

}  // namespace navscene::geo
