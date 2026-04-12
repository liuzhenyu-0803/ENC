#pragma once

#include "data/s57/model.h"

#include <filesystem>
#include <memory>

namespace navscene::data::s57 {

struct ReadRequest {
  std::filesystem::path dataset_path;
  SourceType source_type = SourceType::kChartDataset;
};

class IS57Reader {
 public:
  virtual ~IS57Reader() = default;

  virtual const char* name() const = 0;
  virtual Status Read(const ReadRequest& request, DatasetInfo* out) const = 0;
};

std::unique_ptr<IS57Reader> CreateS57Reader();
std::unique_ptr<IS57Reader> CreateNullS57Reader();

#if defined(NAVSCENE_HAS_GDAL)
std::unique_ptr<IS57Reader> CreateGdalS57Reader();
#endif

Status LoadDataset(const ReadRequest& request, DatasetInfo* out);
const char* PreferredReaderName();
bool HasGdalReader();

}  // namespace navscene::data::s57
