#include "light.h"

u32 LightContainer::addLight(std::unique_ptr<LightData> data)
{
    lightDatas_.push_back(std::move(data));
    return lightDatas_.size() - 1;
}

LightData* LightContainer::getData(u32 id)
{
    if (id >= lightDatas_.size()) return nullptr;
    return lightDatas_[id].get();
}

u32 LightContainer::getContainerSize()
{
    return lightDatas_.size();
}
