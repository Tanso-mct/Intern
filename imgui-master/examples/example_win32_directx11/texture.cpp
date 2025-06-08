#include "texture.h"

using Microsoft::WRL::ComPtr;

u32 TextureContainer::addTexture(std::string path)
{
    for (u32 i = 0; i < textures_.size(); ++i)
    {
        if (textures_[i]->path == path) return i;
    }

    std::unique_ptr<TextureData> data = std::make_unique<TextureData>();
    data->path = path;
    textures_.push_back(std::move(data));

    return textures_.size() - 1;
}

TextureData* TextureContainer::getTexture(u32 id)
{
    if (id >= textures_.size()) return nullptr;
    return textures_[id].get();
}