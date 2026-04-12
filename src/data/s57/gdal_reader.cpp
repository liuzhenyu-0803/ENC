#include "data/s57/reader.h"

#include "data/discovery.h"
#include "data/s57/object_class_catalog.h"
#include "geo/mercator_projection.h"

#include <gdal_priv.h>
#include <ogr_feature.h>
#include <ogrsf_frmts.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace navscene::data::s57 {
namespace {

namespace fs = std::filesystem;

class GdalDatasetHandle {
 public:
  explicit GdalDatasetHandle(GDALDataset* dataset) : dataset_(dataset) {}
  ~GdalDatasetHandle() {
    if (dataset_ != nullptr) {
      GDALClose(dataset_);
    }
  }

  GdalDatasetHandle(const GdalDatasetHandle&) = delete;
  GdalDatasetHandle& operator=(const GdalDatasetHandle&) = delete;

  GDALDataset* get() const { return dataset_; }

 private:
  GDALDataset* dataset_ = nullptr;
};

struct OgrFeatureDeleter {
  void operator()(OGRFeature* feature) const {
    if (feature != nullptr) {
      OGRFeature::DestroyFeature(feature);
    }
  }
};

struct DefaultViewCenterAccumulator {
  double weighted_lon_sum = 0.0;
  double weighted_lat_sum = 0.0;
  double total_weight = 0.0;
};

std::optional<int> FindFieldIndex(const OGRFeature& feature,
                                  std::initializer_list<const char*> candidates) {
  for (const char* candidate : candidates) {
    const int field_index = feature.GetFieldIndex(candidate);
    if (field_index >= 0) {
      return field_index;
    }
  }
  return std::nullopt;
}

std::string GetStringField(const OGRFeature& feature,
                           std::initializer_list<const char*> candidates) {
  const auto field_index = FindFieldIndex(feature, candidates);
  if (!field_index.has_value() || !feature.IsFieldSetAndNotNull(*field_index)) {
    return {};
  }
  return feature.GetFieldAsString(*field_index);
}

int GetIntField(const OGRFeature& feature,
                std::initializer_list<const char*> candidates) {
  const auto field_index = FindFieldIndex(feature, candidates);
  if (!field_index.has_value() || !feature.IsFieldSetAndNotNull(*field_index)) {
    return 0;
  }
  return feature.GetFieldAsInteger(*field_index);
}

std::optional<int> FindAttributeIntValue(const AttributeList& attributes, std::string_view key) {
  for (const auto& [attribute_key, attribute_value] : attributes) {
    if (attribute_key != key || attribute_value.empty()) {
      continue;
    }
    try {
      return std::stoi(attribute_value);
    } catch (...) {
      return std::nullopt;
    }
  }
  return std::nullopt;
}

void PopulateObjectClass(const OGRFeature& feature,
                         int* out_code,
                         std::string* out_name,
                         std::string* out_acronym) {
  if (out_code == nullptr || out_name == nullptr || out_acronym == nullptr) {
    return;
  }

  const int object_class_code = GetIntField(feature, {"OBJL"});
  *out_code = object_class_code;
  out_name->clear();
  out_acronym->clear();

  if (const auto* info = FindObjectClassInfo(object_class_code)) {
    *out_name = info->object_class_name;
    *out_acronym = info->acronym;
  }
}

bool IsDatasetIdentityLayer(std::string_view layer_name) {
  return layer_name == "DSID";
}

bool IsDatasetParametersLayer(std::string_view layer_name) {
  return layer_name == "DSPM";
}

bool IsMetadataLayer(std::string_view layer_name) {
  return IsDatasetIdentityLayer(layer_name) ||
         IsDatasetParametersLayer(layer_name);
}

bool HasRenderableGeometry(OGRwkbGeometryType geometry_type) {
  return wkbFlatten(geometry_type) != wkbNone;
}

std::optional<fs::path> FindExistingS57CsvDir(
    std::initializer_list<fs::path> candidates) {
  for (const auto& candidate : candidates) {
    if (PathExists(candidate / "s57objectclasses.csv") &&
        PathExists(candidate / "s57attributes.csv")) {
      return candidate;
    }
  }
  return std::nullopt;
}

std::optional<fs::path> ResolveS57CsvDir() {
#if defined(NAVSCENE_DEFAULT_GDAL_DATA_PATH)
  if (const auto candidate =
          FindExistingS57CsvDir({fs::path(NAVSCENE_DEFAULT_GDAL_DATA_PATH)});
      candidate.has_value()) {
    return candidate;
  }
#endif

  if (const char* configured = CPLGetConfigOption("S57_CSV", nullptr)) {
    if (const auto candidate = FindExistingS57CsvDir({fs::path(configured)});
        candidate.has_value()) {
      return candidate;
    }
  }

  if (const char* gdal_data = CPLGetConfigOption("GDAL_DATA", nullptr)) {
    if (const auto candidate = FindExistingS57CsvDir({fs::path(gdal_data)});
        candidate.has_value()) {
      return candidate;
    }
  }

  if (const char* osgeo_root = std::getenv("OSGEO4W_ROOT")) {
    if (const auto candidate = FindExistingS57CsvDir({
            fs::path(osgeo_root) / "apps" / "gdal" / "share" / "gdal",
            fs::path(osgeo_root) / "share" / "gdal",
        });
        candidate.has_value()) {
      return candidate;
    }
  }

  return std::nullopt;
}

void ConfigureS57CsvDir() {
  if (const auto s57_csv_dir = ResolveS57CsvDir(); s57_csv_dir.has_value()) {
    CPLSetConfigOption("S57_CSV", s57_csv_dir->string().c_str());
  }
}

AttributeList ReadFeatureAttributes(const OGRFeature& feature) {
  AttributeList attributes;
  const OGRFeatureDefn* definition = feature.GetDefnRef();
  if (definition == nullptr) {
    return attributes;
  }

  const int field_count = definition->GetFieldCount();
  attributes.reserve(static_cast<size_t>(std::max(field_count, 0)));
  for (int field_index = 0; field_index < field_count; ++field_index) {
    const OGRFieldDefn* field = definition->GetFieldDefn(field_index);
    if (field == nullptr || !feature.IsFieldSetAndNotNull(field_index)) {
      continue;
    }

    const char* name = field->GetNameRef();
    if (name == nullptr || *name == '\0') {
      continue;
    }

    const char* value = feature.GetFieldAsString(field_index);
    if (value == nullptr || *value == '\0') {
      continue;
    }

    attributes.emplace_back(std::string(name), std::string(value));
  }

  return attributes;
}

bool AlmostEqual(double lhs, double rhs) {
  return std::abs(lhs - rhs) <= 1e-12;
}

GeoPoint ToGeoPoint(const OGRPoint& point) {
  return GeoPoint{
      .lat = point.getY(),
      .lon = point.getX(),
  };
}

void AppendRingVertices(const OGRLinearRing& ring, std::vector<GeoPoint>* out) {
  if (out == nullptr) {
    return;
  }

  out->clear();
  const int point_count = ring.getNumPoints();
  if (point_count <= 0) {
    return;
  }

  out->reserve(static_cast<size_t>(point_count));
  for (int index = 0; index < point_count; ++index) {
    OGRPoint point;
    ring.getPoint(index, &point);
    out->push_back(ToGeoPoint(point));
  }

  if (out->size() >= 2) {
    const GeoPoint& first = out->front();
    const GeoPoint& last = out->back();
    if (AlmostEqual(first.lat, last.lat) && AlmostEqual(first.lon, last.lon)) {
      out->pop_back();
    }
  }
}

template <typename Visitor>
void VisitChildGeometries(const OGRGeometryCollection& collection, Visitor&& visitor) {
  const int child_count = collection.getNumGeometries();
  for (int child_index = 0; child_index < child_count; ++child_index) {
    if (const auto* child = collection.getGeometryRef(child_index)) {
      visitor(*child);
    }
  }
}

void CollectPoints(const OGRGeometry& geometry, std::vector<PointGeometry>* out) {
  if (out == nullptr) {
    return;
  }

  switch (wkbFlatten(geometry.getGeometryType())) {
    case wkbPoint: {
      const auto* point = geometry.toPoint();
      if (point != nullptr) {
        out->push_back(PointGeometry{.position = ToGeoPoint(*point)});
      }
      return;
    }
    case wkbGeometryCollection: {
      if (const auto* collection = geometry.toGeometryCollection()) {
        VisitChildGeometries(*collection,
                             [&](const OGRGeometry& child) { CollectPoints(child, out); });
      }
      return;
    }
    case wkbMultiPoint: {
      if (const auto* multi_point = geometry.toMultiPoint()) {
        VisitChildGeometries(*multi_point,
                             [&](const OGRGeometry& child) { CollectPoints(child, out); });
      }
      return;
    }
    default:
      return;
  }
}

void CollectLines(const OGRGeometry& geometry,
                  std::vector<LineStringGeometry>* out,
                  uint64_t* vertex_count) {
  if (out == nullptr || vertex_count == nullptr) {
    return;
  }

  switch (wkbFlatten(geometry.getGeometryType())) {
    case wkbLineString: {
      const auto* line = geometry.toLineString();
      if (line == nullptr) {
        return;
      }

      const int point_count = line->getNumPoints();
      if (point_count < 2) {
        return;
      }

      LineStringGeometry segment;
      segment.vertices.reserve(static_cast<size_t>(point_count));
      for (int index = 0; index < point_count; ++index) {
        OGRPoint point;
        line->getPoint(index, &point);
        segment.vertices.push_back(ToGeoPoint(point));
      }
      *vertex_count += static_cast<uint64_t>(segment.vertices.size());
      out->push_back(std::move(segment));
      return;
    }
    case wkbGeometryCollection: {
      if (const auto* collection = geometry.toGeometryCollection()) {
        VisitChildGeometries(*collection, [&](const OGRGeometry& child) {
          CollectLines(child, out, vertex_count);
        });
      }
      return;
    }
    case wkbMultiLineString: {
      if (const auto* multi_line = geometry.toMultiLineString()) {
        VisitChildGeometries(*multi_line, [&](const OGRGeometry& child) {
          CollectLines(child, out, vertex_count);
        });
      }
      return;
    }
    default:
      return;
  }
}

void CollectPolygons(const OGRGeometry& geometry,
                     std::vector<PolygonGeometry>* out,
                     uint64_t* vertex_count) {
  if (out == nullptr || vertex_count == nullptr) {
    return;
  }

  switch (wkbFlatten(geometry.getGeometryType())) {
    case wkbPolygon: {
      const auto* polygon = geometry.toPolygon();
      if (polygon == nullptr) {
        return;
      }

      const auto* exterior_ring = polygon->getExteriorRing();
      if (exterior_ring == nullptr) {
        return;
      }

      PolygonGeometry normalized;
      AppendRingVertices(*exterior_ring, &normalized.outer_ring);
      if (normalized.outer_ring.size() < 3) {
        return;
      }
      *vertex_count += static_cast<uint64_t>(normalized.outer_ring.size());

      const int interior_ring_count = polygon->getNumInteriorRings();
      normalized.holes.reserve(static_cast<size_t>(interior_ring_count));
      for (int ring_index = 0; ring_index < interior_ring_count; ++ring_index) {
        const auto* interior_ring = polygon->getInteriorRing(ring_index);
        if (interior_ring == nullptr) {
          continue;
        }

        std::vector<GeoPoint> hole;
        AppendRingVertices(*interior_ring, &hole);
        if (hole.size() < 3) {
          continue;
        }
        *vertex_count += static_cast<uint64_t>(hole.size());
        normalized.holes.push_back(std::move(hole));
      }

      out->push_back(std::move(normalized));
      return;
    }
    case wkbGeometryCollection: {
      if (const auto* collection = geometry.toGeometryCollection()) {
        VisitChildGeometries(*collection, [&](const OGRGeometry& child) {
          CollectPolygons(child, out, vertex_count);
        });
      }
      return;
    }
    case wkbMultiPolygon: {
      if (const auto* multi_polygon = geometry.toMultiPolygon()) {
        VisitChildGeometries(*multi_polygon, [&](const OGRGeometry& child) {
          CollectPolygons(child, out, vertex_count);
        });
      }
      return;
    }
    default:
      return;
  }
}

bool ComputeRingCentroid(const std::vector<GeoPoint>& ring,
                         GeoPoint* out_centroid,
                         double* out_area) {
  if (out_centroid == nullptr || out_area == nullptr || ring.size() < 3) {
    return false;
  }

  const double reference_lon = ring.front().lon;
  double area_twice = 0.0;
  double centroid_lon_acc = 0.0;
  double centroid_lat_acc = 0.0;
  for (size_t index = 0; index < ring.size(); ++index) {
    const GeoPoint& a = ring[index];
    const GeoPoint& b = ring[(index + 1) % ring.size()];
    const double ax = geo::NormalizeLongitudeDegrees(a.lon, reference_lon);
    const double bx = geo::NormalizeLongitudeDegrees(b.lon, reference_lon);
    const double cross = ax * b.lat - bx * a.lat;
    area_twice += cross;
    centroid_lon_acc += (ax + bx) * cross;
    centroid_lat_acc += (a.lat + b.lat) * cross;
  }

  if (std::abs(area_twice) <= 1e-12) {
    return false;
  }

  out_centroid->lon = centroid_lon_acc / (3.0 * area_twice);
  out_centroid->lat = centroid_lat_acc / (3.0 * area_twice);
  *out_area = std::abs(area_twice) * 0.5;
  return true;
}

bool ComputePolygonCentroid(const PolygonGeometry& polygon,
                            GeoPoint* out_centroid,
                            double* out_area) {
  if (out_centroid == nullptr || out_area == nullptr) {
    return false;
  }

  GeoPoint outer_centroid{};
  double outer_area = 0.0;
  if (!ComputeRingCentroid(polygon.outer_ring, &outer_centroid, &outer_area) || outer_area <= 0.0) {
    return false;
  }

  double weighted_lon_sum = outer_centroid.lon * outer_area;
  double weighted_lat_sum = outer_centroid.lat * outer_area;
  double total_area = outer_area;
  for (const auto& hole : polygon.holes) {
    GeoPoint hole_centroid{};
    double hole_area = 0.0;
    if (!ComputeRingCentroid(hole, &hole_centroid, &hole_area) || hole_area <= 0.0) {
      continue;
    }
    weighted_lon_sum -= hole_centroid.lon * hole_area;
    weighted_lat_sum -= hole_centroid.lat * hole_area;
    total_area -= hole_area;
  }

  if (total_area <= 1e-12) {
    return false;
  }

  out_centroid->lon = weighted_lon_sum / total_area;
  out_centroid->lat = weighted_lat_sum / total_area;
  *out_area = total_area;
  return true;
}

void AccumulateDefaultViewCenter(const AreaFeatureGeometry& area_feature,
                                 DefaultViewCenterAccumulator* accumulator) {
  if (accumulator == nullptr) {
    return;
  }

  const auto catcov = FindAttributeIntValue(area_feature.attributes, "CATCOV");
  if (area_feature.object_class_acronym != "M_COVR" || !catcov.has_value() || *catcov != 1) {
    return;
  }

  for (const auto& polygon : area_feature.polygons) {
    GeoPoint centroid{};
    double area = 0.0;
    if (!ComputePolygonCentroid(polygon, &centroid, &area) || area <= 0.0) {
      continue;
    }

    accumulator->weighted_lon_sum += centroid.lon * area;
    accumulator->weighted_lat_sum += centroid.lat * area;
    accumulator->total_weight += area;
  }
}

void FinalizeGeometrySummary(DatasetInfo* info) {
  if (info == nullptr) {
    return;
  }

  auto& summary = info->geometry.summary;
  summary = {};
  summary.point_feature_count =
      static_cast<uint64_t>(info->geometry.point_features.size());
  summary.line_feature_count =
      static_cast<uint64_t>(info->geometry.line_features.size());
  summary.area_feature_count =
      static_cast<uint64_t>(info->geometry.area_features.size());

  for (const auto& feature : info->geometry.point_features) {
    summary.point_instance_count += static_cast<uint64_t>(feature.points.size());
    summary.vertex_count += static_cast<uint64_t>(feature.points.size());
  }
  for (const auto& feature : info->geometry.line_features) {
    summary.line_part_count += static_cast<uint64_t>(feature.parts.size());
    for (const auto& part : feature.parts) {
      summary.vertex_count += static_cast<uint64_t>(part.vertices.size());
    }
  }
  for (const auto& feature : info->geometry.area_features) {
    summary.area_polygon_count += static_cast<uint64_t>(feature.polygons.size());
    for (const auto& polygon : feature.polygons) {
      summary.vertex_count += static_cast<uint64_t>(polygon.outer_ring.size());
      for (const auto& hole : polygon.holes) {
        summary.vertex_count += static_cast<uint64_t>(hole.size());
      }
    }
  }
}

void TryLoadFeatureGeometry(const std::string& layer_name,
                            OGRLayer* layer,
                            DatasetInfo* info,
                            DefaultViewCenterAccumulator* default_view_center) {
  if (layer == nullptr || info == nullptr) {
    return;
  }

  layer->ResetReading();
  while (auto* raw_feature = layer->GetNextFeature()) {
    std::unique_ptr<OGRFeature, OgrFeatureDeleter> feature(raw_feature);
    OGRGeometry* geometry = feature->GetGeometryRef();
    if (geometry == nullptr) {
      continue;
    }

    PointFeatureGeometry point_feature;
    point_feature.feature_id = feature->GetFID();
    point_feature.source_layer = layer_name;
    PopulateObjectClass(*feature,
                        &point_feature.object_class_code,
                        &point_feature.object_class_name,
                        &point_feature.object_class_acronym);
    point_feature.attributes = ReadFeatureAttributes(*feature);
    CollectPoints(*geometry, &point_feature.points);
    if (!point_feature.points.empty()) {
      info->geometry.point_features.push_back(std::move(point_feature));
    }

    LineFeatureGeometry line_feature;
    line_feature.feature_id = feature->GetFID();
    line_feature.source_layer = layer_name;
    PopulateObjectClass(*feature,
                        &line_feature.object_class_code,
                        &line_feature.object_class_name,
                        &line_feature.object_class_acronym);
    line_feature.attributes = ReadFeatureAttributes(*feature);
    CollectLines(*geometry, &line_feature.parts, &info->geometry.summary.vertex_count);
    if (!line_feature.parts.empty()) {
      info->geometry.line_features.push_back(std::move(line_feature));
    }

    AreaFeatureGeometry area_feature;
    area_feature.feature_id = feature->GetFID();
    area_feature.source_layer = layer_name;
    PopulateObjectClass(*feature,
                        &area_feature.object_class_code,
                        &area_feature.object_class_name,
                        &area_feature.object_class_acronym);
    area_feature.attributes = ReadFeatureAttributes(*feature);
    CollectPolygons(*geometry, &area_feature.polygons, &info->geometry.summary.vertex_count);
    if (!area_feature.polygons.empty()) {
      AccumulateDefaultViewCenter(area_feature, default_view_center);
      info->geometry.area_features.push_back(std::move(area_feature));
    }
  }
}

void UpdateCoverage(OGRLayer* layer,
                    std::optional<OGREnvelope>* merged_extent) {
  if (layer == nullptr || merged_extent == nullptr) {
    return;
  }

  OGREnvelope layer_extent;
  if (layer->GetExtent(&layer_extent, TRUE) != OGRERR_NONE) {
    return;
  }

  if (!merged_extent->has_value()) {
    *merged_extent = layer_extent;
    return;
  }

  merged_extent->value().Merge(layer_extent);
}

void TryReadDatasetMetadata(OGRLayer& layer, DatasetInfo* out) {
  if (out == nullptr) {
    return;
  }

  const std::string layer_name = layer.GetName();
  layer.ResetReading();
  auto feature =
      std::unique_ptr<OGRFeature, OgrFeatureDeleter>(layer.GetNextFeature());
  if (!feature) {
    return;
  }

  if (IsDatasetIdentityLayer(layer_name)) {
    out->dataset_name =
        GetStringField(*feature, {"DSID_DSNM", "DSNM"});
    out->edition = GetStringField(*feature, {"DSID_EDTN", "EDTN"});
    out->update_number = GetStringField(*feature, {"DSID_UPDN", "UPDN"});
    out->issue_date = GetStringField(*feature, {"DSID_ISDT", "ISDT"});
  }

  if (IsDatasetIdentityLayer(layer_name) || IsDatasetParametersLayer(layer_name)) {
    const int compilation_scale =
        std::max(GetIntField(*feature, {"DSPM_CSCL", "CSCL"}), 0);
    if (compilation_scale > 0) {
      out->descriptor.compilation_scale = compilation_scale;
    }
  }

  out->metadata_complete =
      !out->dataset_name.empty() || out->descriptor.compilation_scale > 0;
}

class GdalS57Reader final : public IS57Reader {
 public:
  const char* name() const override { return "gdal-s57"; }

  Status Read(const ReadRequest& request, DatasetInfo* out) const override {
    if (out == nullptr) {
      return Status{StatusCode::kInvalidArgument,
                    "S-57 dataset output pointer must not be null."};
    }
    if (request.dataset_path.empty()) {
      return Status{StatusCode::kInvalidArgument,
                    "S-57 dataset path must not be empty."};
    }
    if (!PathExists(request.dataset_path)) {
      return Status{StatusCode::kNotFound, "S-57 dataset path does not exist."};
    }
    if (!IsRegularFile(request.dataset_path)) {
      return Status{StatusCode::kInvalidArgument,
                    "S-57 dataset path must point to a regular file."};
    }
    if (!HasS57Extension(request.dataset_path)) {
      return Status{StatusCode::kUnsupported,
                    "Only .000 S-57 datasets are recognized in phase 1."};
    }

    GDALAllRegister();
    ConfigureS57CsvDir();

    const std::array<const char*, 2> allowed_drivers = {"S57", nullptr};
    const std::array<const char*, 3> open_options = {
        "UPDATES=APPLY", "RECODE_BY_DSSI=ON", nullptr};

    GdalDatasetHandle dataset(static_cast<GDALDataset*>(
        GDALOpenEx(request.dataset_path.string().c_str(),
                   GDAL_OF_VECTOR | GDAL_OF_READONLY,
                   allowed_drivers.data(),
                   open_options.data(),
                   nullptr)));
    if (dataset.get() == nullptr) {
      return Status{
          StatusCode::kIoError,
          "GDAL failed to open the S-57 dataset. Check that the S57 driver "
          "and runtime CSV catalog are available."};
    }

    DatasetInfo info;
    info.descriptor =
        MakeDatasetDescriptor(request.dataset_path, request.source_type);
    info.reader_name = name();
    DefaultViewCenterAccumulator default_view_center;

    std::optional<OGREnvelope> merged_extent;
    const int layer_count = dataset.get()->GetLayerCount();
    for (int layer_index = 0; layer_index < layer_count; ++layer_index) {
      OGRLayer* layer = dataset.get()->GetLayer(layer_index);
      if (layer == nullptr) {
        continue;
      }

      const std::string layer_name = layer->GetName();
      if (IsMetadataLayer(layer_name)) {
        TryReadDatasetMetadata(*layer, &info);
        continue;
      }

      const auto geometry_type = layer->GetGeomType();
      const bool has_geometry = HasRenderableGeometry(geometry_type);
      const auto raw_feature_count = static_cast<int64_t>(layer->GetFeatureCount(TRUE));
      const auto feature_count =
          static_cast<uint64_t>(std::max<int64_t>(raw_feature_count, 0));

      info.layers.push_back(FeatureLayerInfo{
          .layer_name = layer_name,
          .feature_count = feature_count,
          .has_geometry = has_geometry,
      });
      info.feature_count += feature_count;
      TryLoadFeatureGeometry(layer_name, layer, &info, &default_view_center);

      if (has_geometry) {
        UpdateCoverage(layer, &merged_extent);
      }
    }

    if (merged_extent.has_value()) {
      info.descriptor.coverage.min_lon = merged_extent->MinX;
      info.descriptor.coverage.min_lat = merged_extent->MinY;
      info.descriptor.coverage.max_lon = merged_extent->MaxX;
      info.descriptor.coverage.max_lat = merged_extent->MaxY;
      info.geometry_loaded = true;
    }

    if (info.descriptor.compilation_scale > 0) {
      info.descriptor.default_display_scale = info.descriptor.compilation_scale;
    }
    if (default_view_center.total_weight > 0.0) {
      info.descriptor.default_view_center.lon =
          default_view_center.weighted_lon_sum / default_view_center.total_weight;
      info.descriptor.default_view_center.lat =
          default_view_center.weighted_lat_sum / default_view_center.total_weight;
      info.descriptor.has_default_view_center = true;
    }

    FinalizeGeometrySummary(&info);
    if (info.geometry.summary.point_feature_count > 0 ||
        info.geometry.summary.line_feature_count > 0 ||
        info.geometry.summary.area_feature_count > 0) {
      info.geometry_loaded = true;
    }

    *out = std::move(info);
    return {};
  }
};

}  // namespace

std::unique_ptr<IS57Reader> CreateGdalS57Reader() {
  return std::make_unique<GdalS57Reader>();
}

}  // namespace navscene::data::s57
