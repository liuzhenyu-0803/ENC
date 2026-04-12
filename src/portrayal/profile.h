#pragma once

#include "portrayal/scene.h"

namespace navscene::render {
struct ChartScene;
}

namespace navscene::portrayal {

enum class PortrayalProfileId {
  kS57 = 0,
  kS100,
};

class IPortrayalProfile {
 public:
  virtual ~IPortrayalProfile() = default;

  virtual PortrayalProfileId profile_id() const = 0;
  virtual PortrayalScene BuildScene(const render::ChartScene& scene,
                                    const DisplaySettings& settings) const = 0;
};

const IPortrayalProfile* FindPortrayalProfile(PortrayalProfileId profile_id);
const IPortrayalProfile& DefaultPortrayalProfile();

}  // namespace navscene::portrayal
