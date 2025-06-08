#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include <vector>
#include <memory>
#include <DirectXMath.h>

#include "type.h"

struct LightBuffer
{
    DirectX::XMFLOAT3 LightPos;
    float padding1;
    
    DirectX::XMFLOAT3 LightColor;
    float padding2;

    DirectX::XMFLOAT3 ViewPos;
    float padding3;

    float AmbientStrength;
    int Glossiness;
    float padding[2];
};

struct LightData
{
    LightBuffer srcBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer = nullptr;
};

class LightContainer
{
private:
    std::vector<std::unique_ptr<LightData>> lightDatas_;

public:
    LightContainer() = default;
    ~LightContainer() = default;

    LightContainer(const LightContainer&) = delete;
    LightContainer& operator=(const LightContainer&) = delete;

    u32 addLight(std::unique_ptr<LightData> data);
    LightData* getData(u32 id);

    u32 getContainerSize();
};