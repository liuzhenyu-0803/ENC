#include "geo/mercator_projection.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace navscene::geo {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kDegreesToRadians = kPi / 180.0;
constexpr double kRadiansToDegrees = 180.0 / kPi;
constexpr double kEarthRadiusMeters = 6378137.0;
constexpr double kMercatorLatitudeClampDeg = 89.5;
constexpr double kScaleDenominatorPerMeterPerPixel = 3779.5275590551;

double ClampMercatorLatitude(double lat_deg) {
  return std::clamp(lat_deg, -kMercatorLatitudeClampDeg, kMercatorLatitudeClampDeg);
}

double ScreenOffsetX(double east_px, double north_px, double rotation_rad) {
  return east_px * std::cos(rotation_rad) + north_px * std::sin(rotation_rad);
}

double ScreenOffsetY(double east_px, double north_px, double rotation_rad) {
  return north_px * std::cos(rotation_rad) - east_px * std::sin(rotation_rad);
}

void ScreenToProjectedOffsets(double screen_dx,
                              double screen_dy,
                              double rotation_rad,
                              double* east_px,
                              double* north_px) {
  if (east_px == nullptr || north_px == nullptr) {
    return;
  }

  *east_px = screen_dx * std::cos(rotation_rad) - screen_dy * std::sin(rotation_rad);
  *north_px = screen_dy * std::cos(rotation_rad) + screen_dx * std::sin(rotation_rad);
}

double CoverageCenterLongitude(const GeoBox& coverage) {
  const double max_lon = NormalizeLongitudeDegrees(coverage.max_lon, coverage.min_lon);
  return coverage.min_lon + (max_lon - coverage.min_lon) * 0.5;
}

double CoverageCenterLatitude(const GeoBox& coverage) {
  const double south = MercatorNorthingMeters(coverage.min_lat);
  const double north = MercatorNorthingMeters(coverage.max_lat);
  return InverseMercatorLatitudeDegrees((south + north) * 0.5);
}

}  // namespace

bool HasValidCoverage(const GeoBox& coverage) {
  return coverage.max_lon > coverage.min_lon && coverage.max_lat > coverage.min_lat;
}

double NormalizeLongitudeDegrees(double lon, double reference_lon) {
  double normalized = lon;
  if ((normalized * reference_lon < 0.0) &&
      (std::abs(normalized - reference_lon) > 180.0)) {
    normalized += normalized < 0.0 ? 360.0 : -360.0;
  }

  if (std::abs(normalized - reference_lon) > 180.0) {
    normalized += normalized > reference_lon ? -360.0 : 360.0;
  }
  return normalized;
}

double MercatorNorthingMeters(double lat_deg) {
  const double radians = ClampMercatorLatitude(lat_deg) * kDegreesToRadians;
  return std::log(std::tan(kPi * 0.25 + radians * 0.5)) * kEarthRadiusMeters;
}

double InverseMercatorLatitudeDegrees(double northing_m) {
  return std::atan(std::sinh(northing_m / kEarthRadiusMeters)) * kRadiansToDegrees;
}

double MercatorEastingDeltaMeters(double lon_deg, double reference_lon_deg) {
  return (NormalizeLongitudeDegrees(lon_deg, reference_lon_deg) - reference_lon_deg) *
         kDegreesToRadians * kEarthRadiusMeters;
}

double ComputeCoverageWidthMeters(const GeoBox& coverage) {
  if (!HasValidCoverage(coverage)) {
    return 0.0;
  }

  const double max_lon = NormalizeLongitudeDegrees(coverage.max_lon, coverage.min_lon);
  return std::abs(max_lon - coverage.min_lon) * kDegreesToRadians * kEarthRadiusMeters;
}

double ComputeCoverageHeightMeters(const GeoBox& coverage) {
  if (!HasValidCoverage(coverage)) {
    return 0.0;
  }

  return std::abs(MercatorNorthingMeters(coverage.max_lat) -
                  MercatorNorthingMeters(coverage.min_lat));
}

double ComputeFitPixelsPerMeter(const GeoBox& coverage,
                                uint32_t width,
                                uint32_t height,
                                int padding_pixels) {
  if (!HasValidCoverage(coverage) || width == 0 || height == 0) {
    return 0.0;
  }

  const double draw_width =
      std::max(static_cast<double>(width) - static_cast<double>(padding_pixels * 2), 1.0);
  const double draw_height =
      std::max(static_cast<double>(height) - static_cast<double>(padding_pixels * 2), 1.0);
  const double world_width = ComputeCoverageWidthMeters(coverage);
  const double world_height = ComputeCoverageHeightMeters(coverage);
  if (world_width <= 0.0 || world_height <= 0.0) {
    return 0.0;
  }

  return std::min(draw_width / world_width, draw_height / world_height);
}

Viewport MakeFittedViewport(const GeoBox& coverage,
                            uint32_t width,
                            uint32_t height,
                            double scale_ppm,
                            double rotation_rad,
                            int) {
  Viewport viewport;
  if (!HasValidCoverage(coverage)) {
    return viewport;
  }

  viewport.center.lat = CoverageCenterLatitude(coverage);
  viewport.center.lon = CoverageCenterLongitude(coverage);
  viewport.scale_ppm = scale_ppm;
  viewport.rotation_rad = rotation_rad;
  viewport.width = width;
  viewport.height = height;
  return viewport;
}

ViewportProjection BuildViewportProjection(const GeoBox& coverage,
                                           const Viewport& viewport,
                                           int padding_pixels) {
  ViewportProjection projection;
  if (!HasValidCoverage(coverage) || viewport.width == 0 || viewport.height == 0) {
    return projection;
  }

  projection.draw_width_px =
      std::max(static_cast<double>(viewport.width) - static_cast<double>(padding_pixels * 2), 1.0);
  projection.draw_height_px = std::max(
      static_cast<double>(viewport.height) - static_cast<double>(padding_pixels * 2), 1.0);
  projection.center_x_px = static_cast<double>(padding_pixels) + projection.draw_width_px * 0.5;
  projection.center_y_px = static_cast<double>(padding_pixels) + projection.draw_height_px * 0.5;

  const double fit_pixels_per_meter =
      ComputeFitPixelsPerMeter(coverage, viewport.width, viewport.height, padding_pixels);
  if (fit_pixels_per_meter <= 0.0) {
    return projection;
  }

  projection.pixels_per_meter =
      fit_pixels_per_meter * std::max(viewport.scale_ppm, 0.01);
  projection.valid = projection.pixels_per_meter > 0.0;
  return projection;
}

std::pair<double, double> GeoToPixel(const GeoBox& coverage,
                                     const Viewport& viewport,
                                     const GeoPoint& point,
                                     int padding_pixels) {
  const ViewportProjection projection =
      BuildViewportProjection(coverage, viewport, padding_pixels);
  if (!projection.valid) {
    return {0.0, 0.0};
  }

  const double east_m = MercatorEastingDeltaMeters(point.lon, viewport.center.lon);
  const double north_m = MercatorNorthingMeters(point.lat) -
                         MercatorNorthingMeters(viewport.center.lat);
  const double east_px = east_m * projection.pixels_per_meter;
  const double north_px = north_m * projection.pixels_per_meter;
  return {
      projection.center_x_px + ScreenOffsetX(east_px, north_px, viewport.rotation_rad),
      projection.center_y_px - ScreenOffsetY(east_px, north_px, viewport.rotation_rad),
  };
}

GeoPoint PixelToGeo(const GeoBox& coverage,
                    const Viewport& viewport,
                    double pixel_x,
                    double pixel_y,
                    int padding_pixels) {
  const ViewportProjection projection =
      BuildViewportProjection(coverage, viewport, padding_pixels);
  if (!projection.valid) {
    return viewport.center;
  }

  const double screen_dx = pixel_x - projection.center_x_px;
  const double screen_dy = projection.center_y_px - pixel_y;
  double east_px = 0.0;
  double north_px = 0.0;
  ScreenToProjectedOffsets(
      screen_dx, screen_dy, viewport.rotation_rad, &east_px, &north_px);

  const double east_m = east_px / projection.pixels_per_meter;
  const double north_m = north_px / projection.pixels_per_meter;
  const double center_north_m = MercatorNorthingMeters(viewport.center.lat);
  return GeoPoint{
      .lat = InverseMercatorLatitudeDegrees(center_north_m + north_m),
      .lon = viewport.center.lon +
             (east_m / (kDegreesToRadians * kEarthRadiusMeters)),
  };
}

GeoBox ComputeViewportCoverage(const GeoBox& reference_coverage,
                               const Viewport& viewport,
                               int padding_pixels) {
  if (!HasValidCoverage(reference_coverage) || viewport.width == 0 || viewport.height == 0) {
    return {};
  }

  const std::array<std::pair<double, double>, 4> corners = {
      std::pair<double, double>{static_cast<double>(padding_pixels),
                                static_cast<double>(padding_pixels)},
      std::pair<double, double>{static_cast<double>(viewport.width - padding_pixels),
                                static_cast<double>(padding_pixels)},
      std::pair<double, double>{static_cast<double>(padding_pixels),
                                static_cast<double>(viewport.height - padding_pixels)},
      std::pair<double, double>{static_cast<double>(viewport.width - padding_pixels),
                                static_cast<double>(viewport.height - padding_pixels)},
  };

  GeoBox bounds{
      .min_lat = std::numeric_limits<double>::max(),
      .min_lon = std::numeric_limits<double>::max(),
      .max_lat = std::numeric_limits<double>::lowest(),
      .max_lon = std::numeric_limits<double>::lowest(),
  };

  for (const auto& [x, y] : corners) {
    const GeoPoint point = PixelToGeo(reference_coverage, viewport, x, y, padding_pixels);
    bounds.min_lat = std::min(bounds.min_lat, point.lat);
    bounds.min_lon = std::min(bounds.min_lon, point.lon);
    bounds.max_lat = std::max(bounds.max_lat, point.lat);
    bounds.max_lon = std::max(bounds.max_lon, point.lon);
  }

  return bounds;
}

double EstimateViewportDisplayScale(const GeoBox& viewport_coverage,
                                    const Viewport& viewport,
                                    int padding_pixels) {
  if (!HasValidCoverage(viewport_coverage) || viewport.width == 0 || viewport.height == 0) {
    return 0.0;
  }

  const double draw_width =
      std::max(static_cast<double>(viewport.width) - static_cast<double>(padding_pixels * 2), 1.0);
  const double draw_height = std::max(
      static_cast<double>(viewport.height) - static_cast<double>(padding_pixels * 2), 1.0);
  const double width_m = ComputeCoverageWidthMeters(viewport_coverage);
  const double height_m = ComputeCoverageHeightMeters(viewport_coverage);
  if (width_m <= 0.0 || height_m <= 0.0) {
    return 0.0;
  }

  const double meters_per_pixel =
      std::max(width_m / draw_width, height_m / draw_height);
  return meters_per_pixel * kScaleDenominatorPerMeterPerPixel;
}

}  // namespace navscene::geo
