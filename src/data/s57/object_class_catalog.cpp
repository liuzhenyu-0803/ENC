#include "data/s57/object_class_catalog.h"

#include "data/discovery.h"

#include <cpl_conv.h>
#include <gdal.h>

#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace navscene::data::s57 {
namespace {

namespace fs = std::filesystem;

std::optional<fs::path> FindExistingCatalogPath(
    std::initializer_list<fs::path> candidates) {
  for (const auto& candidate : candidates) {
    if (PathExists(candidate)) {
      return candidate;
    }
  }
  return std::nullopt;
}

std::vector<std::string> ParseCsvFields(std::string_view line) {
  std::vector<std::string> fields;
  std::string current;
  bool in_quotes = false;

  for (size_t index = 0; index < line.size(); ++index) {
    const char ch = line[index];
    if (ch == '"') {
      if (in_quotes && index + 1 < line.size() && line[index + 1] == '"') {
        current.push_back('"');
        ++index;
      } else {
        in_quotes = !in_quotes;
      }
      continue;
    }

    if (ch == ',' && !in_quotes) {
      fields.push_back(current);
      current.clear();
      continue;
    }

    current.push_back(ch);
  }

  fields.push_back(current);
  return fields;
}

std::optional<fs::path> ResolveCatalogPath() {
#if defined(NAVSCENE_DEFAULT_GDAL_DATA_PATH)
  {
    const fs::path candidate =
        fs::path(NAVSCENE_DEFAULT_GDAL_DATA_PATH) / "s57objectclasses.csv";
    if (PathExists(candidate)) {
      return candidate;
    }
  }
#endif

  if (const char* s57_csv = CPLGetConfigOption("S57_CSV", nullptr)) {
    const fs::path candidate = fs::path(s57_csv) / "s57objectclasses.csv";
    if (PathExists(candidate)) {
      return candidate;
    }
  }

  if (const char* gdal_data = CPLGetConfigOption("GDAL_DATA", nullptr)) {
    const fs::path candidate = fs::path(gdal_data) / "s57objectclasses.csv";
    if (PathExists(candidate)) {
      return candidate;
    }
  }

  if (const char* osgeo_root = std::getenv("OSGEO4W_ROOT")) {
    const fs::path candidate =
        fs::path(osgeo_root) / "apps" / "gdal" / "share" / "gdal" /
        "s57objectclasses.csv";
    if (PathExists(candidate)) {
      return candidate;
    }
  }

#if defined(_WIN32)
  HMODULE gdal_module = nullptr;
  if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                             GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         reinterpret_cast<LPCWSTR>(&GDALAllRegister),
                         &gdal_module) != 0 &&
      gdal_module != nullptr) {
    wchar_t buffer[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(gdal_module, buffer, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
      const fs::path module_path(buffer);
      const fs::path bin_dir = module_path.parent_path();
      if (const auto candidate = FindExistingCatalogPath({
              bin_dir / ".." / "apps" / "gdal" / "share" / "gdal" /
                  "s57objectclasses.csv",
              bin_dir / ".." / "share" / "gdal" / "s57objectclasses.csv",
          });
          candidate.has_value()) {
        return candidate;
      }
    }
  }
#endif

  fs::path current = fs::current_path();
  for (int depth = 0; depth < 8; ++depth) {
    if (const auto candidate = FindExistingCatalogPath({
            current / "third_party" / "gdal" / "share" / "gdal" / "s57objectclasses.csv",
            current / "data" / "gdal" / "s57objectclasses.csv",
        });
        candidate.has_value()) {
      return candidate;
    }
    if (!current.has_parent_path()) {
      break;
    }
    current = current.parent_path();
  }

  return std::nullopt;
}

const std::unordered_map<int, ObjectClassInfo>& Catalog() {
  static std::unordered_map<int, ObjectClassInfo> catalog;
  static std::once_flag once;

  std::call_once(once, [] {
    const auto catalog_path = ResolveCatalogPath();
    if (!catalog_path.has_value()) {
      return;
    }

    std::ifstream stream(*catalog_path);
    if (!stream.is_open()) {
      return;
    }

    std::string line;
    bool first_row = true;
    while (std::getline(stream, line)) {
      if (first_row) {
        first_row = false;
        continue;
      }

      const auto fields = ParseCsvFields(line);
      if (fields.size() < 3) {
        continue;
      }

      try {
        const int code = std::stoi(fields[0]);
        catalog.emplace(code, ObjectClassInfo{
                                 .code = code,
                                 .object_class_name = fields[1],
                                 .acronym = fields[2],
                             });
      } catch (...) {
        continue;
      }
    }
  });

  return catalog;
}

}  // namespace

const ObjectClassInfo* FindObjectClassInfo(int code) {
  const auto& catalog = Catalog();
  const auto it = catalog.find(code);
  return it == catalog.end() ? nullptr : &it->second;
}

bool HasObjectClassAcronym(std::string_view acronym, std::string_view expected) {
  return acronym == expected;
}

}  // namespace navscene::data::s57
