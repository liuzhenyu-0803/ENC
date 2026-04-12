#pragma once

#include "navscene/navscene.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace navscene::data {

struct DiscoveryReport {
  std::vector<DatasetDescriptor> datasets;
  std::vector<std::string> missing_directories;
  std::vector<std::string> failed_directories;
};

std::string NormalizePathString(std::string_view path);
bool PathExists(const std::filesystem::path& path);
bool IsDirectory(const std::filesystem::path& path);
bool IsRegularFile(const std::filesystem::path& path);
bool HasS57Extension(const std::filesystem::path& path);

DatasetDescriptor MakeDatasetDescriptor(const std::filesystem::path& path,
                                        SourceType source_type);

Status ValidateChartDatasetSource(const SourceDescriptor& descriptor,
                                  std::string* normalized_uri);

DiscoveryReport DiscoverS57Datasets(
    const std::vector<std::string>& chart_directories);

}  // namespace navscene::data
