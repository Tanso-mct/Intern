#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include <vector>
#include <string>
#include <memory>

#include "type.h"

struct TextureData
{
    std::string path = "";
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view = nullptr;
};

class TextureContainer
{
private:
    std::vector<std::unique_ptr<TextureData>> textures_;

public:
    TextureContainer() = default;
    ~TextureContainer() = default;

    TextureContainer(const TextureContainer&) = delete;
    TextureContainer& operator=(const TextureContainer&) = delete;

    u32 addTexture(std::string path);
    TextureData* getTexture(u32 id);

    u32 getContainerSize() { return textures_.size(); }
};