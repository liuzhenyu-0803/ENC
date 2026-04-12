#include "portrayal/profile.h"

#include "portrayal/catalog.h"
#include "portrayal/engine.h"

namespace navscene::portrayal {
namespace {

class S57PortrayalProfile final : public IPortrayalProfile {
 public:
  PortrayalProfileId profile_id() const override { return PortrayalProfileId::kS57; }

  PortrayalScene BuildScene(const render::ChartScene& scene,
                            const DisplaySettings& settings) const override {
    return BuildPortrayalScene(scene, settings, DefaultS57PortrayalCatalog());
  }
};

}  // namespace

const IPortrayalProfile* FindPortrayalProfile(PortrayalProfileId profile_id) {
  static const S57PortrayalProfile kS57Profile;
  switch (profile_id) {
    case PortrayalProfileId::kS57:
      return &kS57Profile;
    case PortrayalProfileId::kS100:
      return nullptr;
  }
  return nullptr;
}

const IPortrayalProfile& DefaultPortrayalProfile() {
  static const S57PortrayalProfile kS57Profile;
  return kS57Profile;
}

}  // namespace navscene::portrayal
