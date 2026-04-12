#include "data/s57/reader.h"

#include "data/discovery.h"

#include <memory>

namespace navscene::data::s57 {
namespace {

class NullS57Reader final : public IS57Reader {
 public:
  const char* name() const override { return "filesystem-probe"; }

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

    DatasetInfo info;
    info.descriptor =
        MakeDatasetDescriptor(request.dataset_path, request.source_type);
    info.reader_name = name();

    *out = std::move(info);
    return {};
  }
};

}  // namespace

std::unique_ptr<IS57Reader> CreateNullS57Reader() {
  return std::make_unique<NullS57Reader>();
}

}  // namespace navscene::data::s57
