#include "data/discovery.h"

#include <algorithm>
#include <cctype>
#include <system_error>
#include <unordered_set>
#include <utility>

namespace navscene::data {
namespace {

namespace fs = std::filesystem;

Status ErrorStatus(StatusCode code, std::string message) {
  return Status{code, std::move(message)};
}

std::string ToLowerAscii(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

}  // namespace

std::string NormalizePathString(std::string_view path) {
  std::error_code ec;
  const fs::path raw_path{std::string(path)};
  const fs::path normalized = fs::weakly_canonical(raw_path, ec);
  if (!ec) {
    return normalized.string();
  }
  return raw_path.lexically_normal().string();
}

bool PathExists(const fs::path& path) {
  std::error_code ec;
  return fs::exists(path, ec);
}

bool IsDirectory(const fs::path& path) {
  std::error_code ec;
  return fs::is_directory(path, ec);
}

bool IsRegularFile(const fs::path& path) {
  std::error_code ec;
  return fs::is_regular_file(path, ec);
}

bool HasS57Extension(const fs::path& path) {
  return ToLowerAscii(path.extension().string()) == ".000";
}

DatasetDescriptor MakeDatasetDescriptor(const fs::path& path,
                                        SourceType source_type) {
  DatasetDescriptor descriptor;
  descriptor.id = NormalizePathString(path.string());
  descriptor.path = descriptor.id;
  descriptor.format = DatasetFormat::kS57;
  descriptor.family = ProductFamily::kEnc;
  descriptor.source_type = source_type;
  return descriptor;
}

Status ValidateChartDatasetSource(const SourceDescriptor& descriptor,
                                  std::string* normalized_uri) {
  if (descriptor.uri.empty()) {
    return ErrorStatus(StatusCode::kInvalidArgument,
                       "Chart dataset source URI must not be empty.");
  }

  if (descriptor.format != DatasetFormat::kUnknown &&
      descriptor.format != DatasetFormat::kS57) {
    return ErrorStatus(StatusCode::kUnsupported,
                       "Only S-57 chart dataset sources are supported in phase 1.");
  }

  const fs::path path{descriptor.uri};
  if (!PathExists(path)) {
    return ErrorStatus(StatusCode::kNotFound,
                       "Chart dataset source path does not exist.");
  }
  if (IsDirectory(path)) {
    return ErrorStatus(
        StatusCode::kInvalidArgument,
        "Chart directories must be registered through ICatalog::AddChartDirectory.");
  }
  if (!IsRegularFile(path)) {
    return ErrorStatus(StatusCode::kInvalidArgument,
                       "Chart dataset source must be a regular file.");
  }
  if (!HasS57Extension(path)) {
    return ErrorStatus(StatusCode::kUnsupported,
                       "Only .000 S-57 datasets are recognized in phase 1.");
  }

  if (normalized_uri != nullptr) {
    *normalized_uri = NormalizePathString(path.string());
  }
  return {};
}

DiscoveryReport DiscoverS57Datasets(
    const std::vector<std::string>& chart_directories) {
  DiscoveryReport report;
  std::unordered_set<std::string> seen_paths;

  for (const auto& directory : chart_directories) {
    const fs::path directory_path{directory};
    if (!PathExists(directory_path)) {
      report.missing_directories.push_back(directory);
      continue;
    }
    if (!IsDirectory(directory_path)) {
      report.failed_directories.push_back(directory);
      continue;
    }

    std::error_code ec;
    fs::recursive_directory_iterator it(
        directory_path, fs::directory_options::skip_permission_denied, ec);
    if (ec) {
      report.failed_directories.push_back(directory);
      continue;
    }

    for (const auto& entry : it) {
      std::error_code file_ec;
      if (!entry.is_regular_file(file_ec) || file_ec) {
        continue;
      }
      if (!HasS57Extension(entry.path())) {
        continue;
      }

      auto dataset = MakeDatasetDescriptor(entry.path(), SourceType::kChartDataset);
      if (seen_paths.insert(dataset.path).second) {
        report.datasets.push_back(std::move(dataset));
      }
    }
  }

  return report;
}

}  // namespace navscene::data
