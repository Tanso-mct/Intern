#pragma once

#include <d3d11.h>
#include <memory>
#include <Windows.h>
#include <string>
#include <string_view>
#include <vector>

#include <DirectXMath.h>
#include <wrl/client.h>

#include "type.h"

struct Vertex;
class TextureContainer;

enum COLOR_FILTER
{
    COLOR_FILTER_NONE = 0,
    COLOR_FILTER_SEPIA = 1,
    COLOR_FILTER_GRAY_SCALE = 2,
};

enum LIGHTING_MODE
{
    LIGHTING_NONE = 0,
    LIGHTING_NORMAL_BASE,
    LIGHTING_PHONG,
};

class VisualObject
{
private:
    std::string name_ = "";

    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuff_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuff_;
    u32 indicesCount_;

public:
    VisualObject
    (
        std::string name, bool needsDisplayed,
        Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuff, Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuff,
        u32 indicesCount
    ) : name_(name), needsDisplayed_(needsDisplayed)
    , vertexBuff_(vertexBuff), indexBuff_(indexBuff), indicesCount_(indicesCount)
    , pos_(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f))
    , scale_(DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f))
    , rotate_(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)){}

    virtual ~VisualObject() = default;
    bool needsDisplayed_ = false;

    DirectX::XMFLOAT3 pos_;
    DirectX::XMFLOAT3 scale_;
    DirectX::XMFLOAT3 rotate_;

    COLOR_FILTER colorFilter_ = COLOR_FILTER_NONE;
    LIGHTING_MODE useLightingMode_ = LIGHTING_NONE;

    std::string getName();

    void setVertexBuff(ID3D11DeviceContext* d3dDeviceContext);
    void setIndexBuff(ID3D11DeviceContext* d3dDeviceContext);

    HRESULT updateAffineMat(ID3D11DeviceContext* d3dDeviceContext, Microsoft::WRL::ComPtr<ID3D11Buffer> affineMatBuff);
    HRESULT updateCBScene(ID3D11DeviceContext* d3dDeviceContext, Microsoft::WRL::ComPtr<ID3D11Buffer> cbScene);

    virtual HRESULT setTexture(ID3D11DeviceContext* d3dDeviceContext, TextureContainer& textureContainer, u32 slot) = 0;

    void drawIndex(ID3D11DeviceContext* d3dDeviceContext);
};

class PlainObject : public VisualObject
{
public:
    PlainObject
    (
        std::string name, bool needsDisplayed,
        Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuff, Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuff,
        u32 indicesCount
    ) : VisualObject(name, needsDisplayed, vertexBuff, indexBuff, indicesCount){}

    ~PlainObject() final = default;

    HRESULT setTexture(ID3D11DeviceContext* d3dDeviceContext, TextureContainer& textureContainer, u32 slot) final;
};

class TexturedObject : public VisualObject
{
private:
    u32 textureId_;

public:
    TexturedObject
    (
        std::string name, bool needsDisplayed, u32 textureId,
        Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuff, Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuff,
        u32 indicesCount
    ) : VisualObject(name, needsDisplayed, vertexBuff, indexBuff, indicesCount), textureId_(textureId){}
    
    ~TexturedObject() final = default;

    HRESULT setTexture(ID3D11DeviceContext* d3dDeviceContext, TextureContainer& textureContainer, u32 slot) final;
};

class ObjectContainer
{
private:
    std::vector<std::unique_ptr<VisualObject>> objectDatas_;

public:
    ObjectContainer() = default;
    ~ObjectContainer() = default;

    ObjectContainer(const ObjectContainer&) = delete;
    ObjectContainer& operator=(const ObjectContainer&) = delete;

    u32 addObject(std::unique_ptr<VisualObject> object);
    VisualObject* getData(u32 id);

    u32 getContainerSize();
};