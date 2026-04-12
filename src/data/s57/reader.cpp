#include "data/s57/reader.h"

namespace navscene::data::s57 {

std::unique_ptr<IS57Reader> CreateS57Reader() {
#if defined(NAVSCENE_HAS_GDAL)
  return CreateGdalS57Reader();
#else
  return CreateNullS57Reader();
#endif
}

Status LoadDataset(const ReadRequest& request, DatasetInfo* out) {
  if (out == nullptr) {
    return Status{StatusCode::kInvalidArgument,
                  "S-57 dataset output pointer must not be null."};
  }

  auto reader = CreateS57Reader();
  return reader->Read(request, out);
}

const char* PreferredReaderName() {
  auto reader = CreateS57Reader();
  return reader->name();
}

bool HasGdalReader() {
#if defined(NAVSCENE_HAS_GDAL)
  return true;
#else
  return false;
#endif
}

}  // namespace navscene::data::s57
