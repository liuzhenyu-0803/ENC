#pragma once

#include "portrayal/catalog.h"

namespace navscene::portrayal {

PortrayalScene BuildPortrayalScene(
    const render::ChartScene& scene,
    const DisplaySettings& settings,
    const S57PortrayalCatalog& catalog = DefaultS57PortrayalCatalog());

PortrayalScene BuildPortrayalScene(const render::ChartScene& scene,
                                   const DisplaySettings& settings,
                                   const IPortrayalProfile& profile);

PortrayalScene BuildPortrayalScene(const render::ChartScene& scene,
                                   const DisplaySettings& settings,
                                   PortrayalProfileId profile_id);

}  // namespace navscene::portrayal
