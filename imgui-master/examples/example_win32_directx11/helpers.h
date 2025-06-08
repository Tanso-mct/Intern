#pragma once

#include <d3d11.h>
#include <Windows.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <array>

#include "type.h"

class Converter;
class TextureContainer;

class VisualObject;
class FullScreenQuad;
class ObjectContainer;

struct LightBuffer;
class LightContainer;
enum LIGHTING_MODE;

struct Vertex //インプットレイアウト
{
    DirectX::XMFLOAT3 Pos; //頂点座標
    DirectX::XMFLOAT2 Tex; //UV座標
    DirectX::XMFLOAT3 Normal; //法線ベクトル
    DirectX::XMFLOAT4 Col; //頂点カラー ARGB

    // テクスチャを使用するためカラースケールは1.0f。0.0fにするとテクスチャが反映されない
    DirectX::XMFLOAT4 ColScale;
};

struct CBScene
{
    int colorFilterType;
    int useLightingType;
    float padding[2];
};

struct CBBloomPrams
{
    float threshold;
    DirectX::XMFLOAT2 texSize;
    float intensity;
    float blurScale;
    float padding[3];
};

struct RenderTarget
{
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> view;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
};

struct TupleHash 
{
    template <typename T1, typename T2>
    std::size_t operator()(const std::tuple<T1, T2>& t) const {
        return std::hash<T1>{}(std::get<0>(t)) ^ (std::hash<T2>{}(std::get<1>(t)) << 1);
    }
};

std::unique_ptr<u8[]> LoadFile(std::string path, fpos_t& size);
DirectX::XMUINT2 GetClientSize(HWND hWnd);

Microsoft::WRL::ComPtr<ID3D11Device>& D3DDevice();

HRESULT CreateDepthStencil
(
    Microsoft::WRL::ComPtr<ID3D11Texture2D>& depthStencil, 
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& depthStencilView, 
    DirectX::XMUINT2 clientSize
);

HRESULT CreateVertexShader
(
    std::unique_ptr<u8[]>& compiledShader, u32 size, 
    Microsoft::WRL::ComPtr<ID3D11VertexShader>& vertexShader
);
HRESULT CreatePixelShader
(
    std::unique_ptr<u8[]>& compiledShader, 
    u32 size, Microsoft::WRL::ComPtr<ID3D11PixelShader>& pixelShader
);

HRESULT CreateInputLayout
(
    std::unique_ptr<u8[]>& compiledShader, u32 size, 
    Microsoft::WRL::ComPtr<ID3D11InputLayout>& inputLayout
);

HRESULT CreateWVPMatrix
(
    Microsoft::WRL::ComPtr<ID3D11Buffer>& worldMatBuff, 
    Microsoft::WRL::ComPtr<ID3D11Buffer>& viewMatBuff, 
    Microsoft::WRL::ComPtr<ID3D11Buffer>& projMatBuff, 
    ID3D11DeviceContext* d3dDeviceContext,
    DirectX::XMUINT2 clientSize
);
HRESULT CreateAffineMatrix(Microsoft::WRL::ComPtr<ID3D11Buffer>& affineMatBuff, ID3D11DeviceContext* d3dDeviceContext);

HRESULT CreateBlendState(Microsoft::WRL::ComPtr<ID3D11BlendState>& blendState);
HRESULT CreateSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState>& samplerState);

HRESULT CreateConstantBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>& constantBuffer);

HRESULT CreateCBBloomPramsBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>& cbBloomPramsBuffer, CBBloomPrams& cbData);
HRESULT UpdateCBBloomPramsBuffer(ID3D11DeviceContext* d3dDeviceContext, Microsoft::WRL::ComPtr<ID3D11Buffer>& cbBloomPramsBuffer, CBBloomPrams& cbData);

void SetViewPort(DirectX::XMUINT2 clientSize, ID3D11DeviceContext* d3dDeviceContext);

HRESULT ShowDebugWindow
(
    ID3D11DeviceContext* d3dDeviceContext,
    ObjectContainer& objectContainer, int& selectingSprite,
    LightContainer& lightContainer, int& selectingLight,
    bool& usingBloom, Microsoft::WRL::ComPtr<ID3D11Buffer> cbBloomPramsBuffer, CBBloomPrams& bloomParams
);

Microsoft::WRL::ComPtr<ID3D11Buffer> CreateVertexBuffer(Vertex* vertices, u32 vertexSize);
Microsoft::WRL::ComPtr<ID3D11Buffer> CreateIndexBuffer(u32* indices, u32 indexSize);

HRESULT CreateTextures(Converter& converter, TextureContainer& container);
HRESULT CreateTextureBuffer
(
    Microsoft::WRL::ComPtr<ID3D11Texture2D>& texture,
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& view,
    DirectX::XMUINT2 clientSize, std::unique_ptr<u8[]>& pixels
);

HRESULT CreateObjects(ObjectContainer& container);
std::unique_ptr<VisualObject> CreateFullScreenTriangle();

DirectX::XMFLOAT3 GetFloat3FromChar(u8*& start, u8* end, char divideChar, char endChar);
DirectX::XMINT3 GetInt3FromChar(u8*& start, u8* end, char divideChar, char endChar);
DirectX::XMFLOAT2 GetFloat2FromChar(u8*& start, u8* end, char divideChar, char endChar);

struct MemRange
{
    u8* start;
    u8* end;
};
std::array<MemRange, 3> GetMemRange3FromChar(u8*& start, u8* end, char divideChar, char endChar);

HRESULT CreateObjectFromObjFile
(
    std::unique_ptr<u8[]>& buff, fpos_t& size, 
    std::string name, bool needsDisplayed, LIGHTING_MODE lightingMode,
    ObjectContainer& container
);

HRESULT CreateLights(LightContainer& container);
HRESULT CreateLightBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>& lightBuffer, LightBuffer& lightData);
void SetLightBuffer(ID3D11DeviceContext* d3dDeviceContext, Microsoft::WRL::ComPtr<ID3D11Buffer> &lightBuffer);
void UpdateLightBuffer
(
    ID3D11DeviceContext *d3dDeviceContext, 
    Microsoft::WRL::ComPtr<ID3D11Buffer> &lightBuffer, LightBuffer &lightData
);

HRESULT CreateRenderTarget(DirectX::XMUINT2 clientSize, RenderTarget& renderTarget);