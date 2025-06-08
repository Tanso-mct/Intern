#include "visual_object.h"

#include "texture.h"
#include "helpers.h"

using Microsoft::WRL::ComPtr;

std::string VisualObject::getName()
{
    return name_;
}

void VisualObject::setVertexBuff(ID3D11DeviceContext *d3dDeviceContext)
{
    u32 stride = sizeof(Vertex);
    u32 offset = 0;
    d3dDeviceContext->IASetVertexBuffers(0, 1, vertexBuff_.GetAddressOf(), &stride, &offset);
}

void VisualObject::setIndexBuff(ID3D11DeviceContext *d3dDeviceContext)
{
    d3dDeviceContext->IASetIndexBuffer(indexBuff_.Get(), DXGI_FORMAT_R32_UINT, 0);
}

HRESULT VisualObject::updateAffineMat(ID3D11DeviceContext *d3dDeviceContext, ComPtr<ID3D11Buffer> affineMatBuff)
{
    HRESULT hr = S_OK;

    DirectX::XMMATRIX affineMat = DirectX::XMMatrixIdentity();
    affineMat *= DirectX::XMMatrixScaling(scale_.x, scale_.y, scale_.z); //倍率
    affineMat *= DirectX::XMMatrixRotationX(rotate_.x); //回転
    affineMat *= DirectX::XMMatrixRotationY(rotate_.y);
    affineMat *= DirectX::XMMatrixRotationZ(rotate_.z);
    affineMat *= DirectX::XMMatrixTranslation(pos_.x, pos_.y, pos_.z); //移動

    affineMat = XMMatrixTranspose(affineMat); //転置

    D3D11_MAPPED_SUBRESOURCE affineMappedResource = { 0 };
    hr = d3dDeviceContext->Map(affineMatBuff.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &affineMappedResource);
    if (FAILED(hr)) return hr;
    
    memcpy_s(affineMappedResource.pData, sizeof(DirectX::XMMATRIX), &affineMat, sizeof(DirectX::XMMATRIX));
    d3dDeviceContext->Unmap(affineMatBuff.Get(), 0);

    return hr;
}

HRESULT VisualObject::updateCBScene(ID3D11DeviceContext *d3dDeviceContext, ComPtr<ID3D11Buffer> cbScene)
{
    HRESULT hr = S_OK;

    CBScene cbSceneData;
    cbSceneData.colorFilterType = (s32)colorFilter_;
    cbSceneData.useLightingType = (s32)useLightingMode_;

    D3D11_MAPPED_SUBRESOURCE cbMappedResource = { 0 };
    hr = d3dDeviceContext->Map(cbScene.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cbMappedResource);
    if (FAILED(hr)) return hr;

    memcpy_s(cbMappedResource.pData, sizeof(CBScene), &cbSceneData, sizeof(CBScene));
    d3dDeviceContext->Unmap(cbScene.Get(), 0);

    return hr;
}

void VisualObject::drawIndex(ID3D11DeviceContext *d3dDeviceContext)
{
    d3dDeviceContext->DrawIndexed
    (
        indicesCount_, // 描画するインデックスの数を指定
        0, 0
    );
}

u32 ObjectContainer::addObject(std::unique_ptr<VisualObject> object)
{
    objectDatas_.push_back(std::move(object));
    return objectDatas_.size() - 1;
}

VisualObject* ObjectContainer::getData(u32 id)
{
    if (id >= objectDatas_.size()) return nullptr;
    return objectDatas_[id].get();
}

u32 ObjectContainer::getContainerSize()
{
    return objectDatas_.size();
}

HRESULT PlainObject::setTexture(ID3D11DeviceContext *d3dDeviceContext, TextureContainer &textureContainer, u32 slot)
{
    TextureData* texture = textureContainer.getTexture(0);
    if (!texture) return E_FAIL;

    d3dDeviceContext->PSSetShaderResources(slot, 1, texture->view.GetAddressOf());
    return S_OK;
}

HRESULT TexturedObject::setTexture(ID3D11DeviceContext *d3dDeviceContext, TextureContainer &textureContainer, u32 slot)
{
    TextureData* texture = textureContainer.getTexture(textureId_);
    if (!texture) return E_FAIL;

    d3dDeviceContext->PSSetShaderResources(slot, 1, texture->view.GetAddressOf());
    return S_OK;
}
