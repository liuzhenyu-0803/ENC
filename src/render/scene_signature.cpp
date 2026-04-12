#include "render/scene_signature.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <string_view>

namespace navscene::render {
namespace {

constexpr uint64_t kFnvOffsetBasis = 14695981039346656037ull;
constexpr uint64_t kFnvPrime = 1099511628211ull;

struct BucketKey {
  char primitive_kind = '?';
  std::string object_class_acronym;

  bool operator<(const BucketKey& other) const {
    if (primitive_kind != other.primitive_kind) {
      return primitive_kind < other.primitive_kind;
    }
    return object_class_acronym < other.object_class_acronym;
  }
};

struct BucketValue {
  uint64_t primitive_count = 0;
  uint64_t vertex_count = 0;
};

void HashBytes(std::string_view text, uint64_t* hash) {
  if (hash == nullptr) {
    return;
  }

  for (unsigned char ch : text) {
    *hash ^= ch;
    *hash *= kFnvPrime;
  }
}

void HashInt64(int64_t value, uint64_t* hash) {
  char buffer[32];
  const int count = std::snprintf(buffer, sizeof(buffer), "%lld",
                                  static_cast<long long>(value));
  if (count > 0) {
    HashBytes(std::string_view(buffer, static_cast<size_t>(count)), hash);
  }
}

void HashUint64(uint64_t value, uint64_t* hash) {
  char buffer[32];
  const int count = std::snprintf(buffer, sizeof(buffer), "%llu",
                                  static_cast<unsigned long long>(value));
  if (count > 0) {
    HashBytes(std::string_view(buffer, static_cast<size_t>(count)), hash);
  }
}

int64_t QuantizeCoordinate(double value) {
  return static_cast<int64_t>(std::llround(value * 1'000'000.0));
}

void MergePointIntoCoverage(const GeoPoint& point, ChartSceneSignature* signature) {
  if (signature == nullptr) {
    return;
  }

  if (!signature->has_coverage) {
    signature->coverage.min_lat = point.lat;
    signature->coverage.max_lat = point.lat;
    signature->coverage.min_lon = point.lon;
    signature->coverage.max_lon = point.lon;
    signature->has_coverage = true;
    return;
  }

  signature->coverage.min_lat = std::min(signature->coverage.min_lat, point.lat);
  signature->coverage.max_lat = std::max(signature->coverage.max_lat, point.lat);
  signature->coverage.min_lon = std::min(signature->coverage.min_lon, point.lon);
  signature->coverage.max_lon = std::max(signature->coverage.max_lon, point.lon);
}

void AddBucket(std::map<BucketKey, BucketValue>* buckets,
               char primitive_kind,
               const std::string& object_class_acronym,
               uint64_t primitive_count,
               uint64_t vertex_count) {
  if (buckets == nullptr) {
    return;
  }

  auto& bucket = (*buckets)[BucketKey{primitive_kind, object_class_acronym}];
  bucket.primitive_count += primitive_count;
  bucket.vertex_count += vertex_count;
}

}  // namespace

ChartSceneSignature BuildChartSceneSignature(const ChartScene& scene) {
  ChartSceneSignature signature;
  signature.point_primitive_count = static_cast<uint64_t>(scene.points.size());
  signature.polyline_primitive_count = static_cast<uint64_t>(scene.polylines.size());
  signature.polygon_primitive_count = static_cast<uint64_t>(scene.polygons.size());

  std::map<BucketKey, BucketValue> buckets;

  for (const auto& point : scene.points) {
    signature.total_vertex_count += 1;
    MergePointIntoCoverage(point.position, &signature);
    AddBucket(&buckets, 'P', point.object_class_acronym, 1, 1);
  }

  for (const auto& polyline : scene.polylines) {
    const uint64_t vertex_count = static_cast<uint64_t>(polyline.vertices.size());
    signature.total_vertex_count += vertex_count;
    for (const auto& point : polyline.vertices) {
      MergePointIntoCoverage(point, &signature);
    }
    AddBucket(&buckets, 'L', polyline.object_class_acronym, 1, vertex_count);
  }

  for (const auto& polygon : scene.polygons) {
    uint64_t vertex_count = static_cast<uint64_t>(polygon.outer_ring.size());
    for (const auto& point : polygon.outer_ring) {
      MergePointIntoCoverage(point, &signature);
    }
    for (const auto& hole : polygon.holes) {
      vertex_count += static_cast<uint64_t>(hole.size());
      for (const auto& point : hole) {
        MergePointIntoCoverage(point, &signature);
      }
    }

    signature.total_vertex_count += vertex_count;
    AddBucket(&buckets, 'A', polygon.object_class_acronym, 1, vertex_count);
  }

  signature.class_buckets.reserve(buckets.size());
  for (const auto& [key, value] : buckets) {
    signature.class_buckets.push_back(ChartSceneClassBucket{
        .primitive_kind = key.primitive_kind,
        .object_class_acronym = key.object_class_acronym,
        .primitive_count = value.primitive_count,
        .vertex_count = value.vertex_count,
    });
  }

  uint64_t hash = kFnvOffsetBasis;
  HashUint64(signature.point_primitive_count, &hash);
  HashBytes("|", &hash);
  HashUint64(signature.polyline_primitive_count, &hash);
  HashBytes("|", &hash);
  HashUint64(signature.polygon_primitive_count, &hash);
  HashBytes("|", &hash);
  HashUint64(signature.total_vertex_count, &hash);
  HashBytes("|", &hash);
  HashUint64(signature.has_coverage ? 1 : 0, &hash);
  if (signature.has_coverage) {
    HashBytes("|", &hash);
    HashInt64(QuantizeCoordinate(signature.coverage.min_lat), &hash);
    HashBytes("|", &hash);
    HashInt64(QuantizeCoordinate(signature.coverage.min_lon), &hash);
    HashBytes("|", &hash);
    HashInt64(QuantizeCoordinate(signature.coverage.max_lat), &hash);
    HashBytes("|", &hash);
    HashInt64(QuantizeCoordinate(signature.coverage.max_lon), &hash);
  }
  for (const auto& bucket : signature.class_buckets) {
    HashBytes("|", &hash);
    HashBytes(std::string_view(&bucket.primitive_kind, 1), &hash);
    HashBytes(":", &hash);
    HashBytes(bucket.object_class_acronym, &hash);
    HashBytes(":", &hash);
    HashUint64(bucket.primitive_count, &hash);
    HashBytes(":", &hash);
    HashUint64(bucket.vertex_count, &hash);
  }
  signature.fingerprint64 = hash;

  return signature;
}

std::string FormatChartSceneSignature(const ChartSceneSignature& signature) {
  std::string text;
  text += "fingerprint=" + std::to_string(signature.fingerprint64);
  text += " points=" + std::to_string(signature.point_primitive_count);
  text += " lines=" + std::to_string(signature.polyline_primitive_count);
  text += " areas=" + std::to_string(signature.polygon_primitive_count);
  text += " vertices=" + std::to_string(signature.total_vertex_count);
  if (signature.has_coverage) {
    char buffer[160];
    const int count = std::snprintf(buffer,
                                    sizeof(buffer),
                                    " coverage=[%.6f,%.6f -> %.6f,%.6f]",
                                    signature.coverage.min_lat,
                                    signature.coverage.min_lon,
                                    signature.coverage.max_lat,
                                    signature.coverage.max_lon);
    if (count > 0) {
      text.append(buffer, static_cast<size_t>(count));
    }
  }
  for (const auto& bucket : signature.class_buckets) {
    text += " ";
    text.push_back(bucket.primitive_kind);
    text += ":";
    text += bucket.object_class_acronym.empty() ? "<empty>" : bucket.object_class_acronym;
    text += "=" + std::to_string(bucket.primitive_count);
    text += "/" + std::to_string(bucket.vertex_count);
  }
  return text;
}

}  // namespace navscene::render
