#include "helpers.h"

#include <iostream>
#include <tuple>
#include <unordered_map>

using Microsoft::WRL::ComPtr;

#include "converter.h"
#include "pixel_flipper.h"
#include "format_bmp.h"
#include "format_tga.h"
#include "format_dds.h"

#include "texture.h"
#include "visual_object.h"
#include "light.h"
#include "slot_indicies.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"


ComPtr<ID3D11Device> &D3DDevice()
{
    static ComPtr<ID3D11Device> device;
    return device;
}

DirectX::XMUINT2 GetClientSize(HWND hWnd)
{
    RECT rect;
    GetClientRect(hWnd, &rect);
    return DirectX::XMUINT2(rect.right - rect.left, rect.bottom - rect.top);
}

HRESULT CreateDepthStencil
(
    ComPtr<ID3D11Texture2D> &depthStencil, ComPtr<ID3D11DepthStencilView> &depthStencilView, 
    DirectX::XMUINT2 clientSize
){
    // DepthStencilTextureの作成
    D3D11_TEXTURE2D_DESC descDepth = {}; //テクスチャもまずはディスクリプタ
    descDepth.Width = clientSize.x;
    descDepth.Height = clientSize.y;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; //24bit符号なし浮動小数+8bit符号なし整数
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL; //DEPTH+STENCILとしてバインド
    descDepth.CPUAccessFlags = 0; // CPUからの直接アクセスは不要
    descDepth.MiscFlags = 0;

    // テクスチャの作成
    HRESULT hr = D3DDevice()->CreateTexture2D(&descDepth, nullptr, &depthStencil);

    if (FAILED(hr)) return hr; //作成に失敗したので終わり

    // DepthStencilViewの作成
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {}; //ディスクリプタも違うので注意
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;

    // ID3D11Texture2Dオブジェクトをデプスバッファとして使用するために、ID3D11Deviceに登録
    hr = D3DDevice()->CreateDepthStencilView
    (
        depthStencil.Get(), 
        &descDSV,
        &depthStencilView
    );

    if (FAILED(hr)) return hr; //デプスステンシルビュー作成失敗

    return S_OK;
}

std::unique_ptr<u8[]> LoadFile(std::string path, fpos_t& size)
{
    FILE* fp = nullptr;
	errno_t error;

	error = fopen_s(&fp, path.data(), "rb");

	if (error != 0)
    {
        std::cout << "ファイルを読み込めませんでした。" << std::endl;
        return nullptr;
    }

	//ファイルの末尾へ移動して、サイズを計算
	fseek(fp, 0L, SEEK_END);
	fgetpos(fp, &size);

	//ファイルの最初に移動
	fseek(fp, 0L, SEEK_SET);

	std::unique_ptr<u8[]> buff = std::make_unique<u8[]>(size);

	//サイズ分のデータを読み込む
	fread(buff.get(), 1, size, fp);
	fclose(fp);

    return buff;
}

HRESULT CreateVertexShader(std::unique_ptr<u8[]>& compiledShader, u32 size, ComPtr<ID3D11VertexShader> &vertexShader)
{
    // 頂点シェーダの作成
    HRESULT hr = D3DDevice()->CreateVertexShader
    (
        compiledShader.get(), // バイナリデータのポインタ
        size,  // バイナリデータのサイズ
        nullptr, 
        &vertexShader // 作成された頂点シェーダの格納先
    );

    return hr;
}

HRESULT CreateInputLayout(std::unique_ptr<u8[]>& compiledShader, u32 size, ComPtr<ID3D11InputLayout>& inputLayout)
{
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { 
            "POSITION", // Header.hlsliのVS_INPUTで定義した変数名
            0, // 読み込むレイヤーのインデックス
            DXGI_FORMAT_R32G32B32_FLOAT, // 32bitの浮動小数点数の3つ組
            0, // シェーダーステージのこと
            0, // 読み込みバッファの0バイト目から読み込む
            D3D11_INPUT_PER_VERTEX_DATA, // 入力されるデータの種類
            0 // データの繰り返し回数。頂点データの場合は絶対に０
        },
        { 
            "TEXCOORD", // セマンティクスの文字部分
            0, // セマンティクスの数値部分
            DXGI_FORMAT_R32G32_FLOAT, // 32bitの浮動小数点数の2つ組
            0, 
            12, // バッファの読み込みの先頭の場所を示す。本来ならFloat4なため12バイト目(12/3=4)から読み込む
            D3D11_INPUT_PER_VERTEX_DATA, 
            0 
        },
        {
            "NORMAL",
            0,
            DXGI_FORMAT_R32G32B32_FLOAT, // 32bitの浮動小数点数の3つ組
            0,
            D3D11_APPEND_ALIGNED_ELEMENT, // バッファの読み込みの先頭の場所を示す。
            D3D11_INPUT_PER_VERTEX_DATA,
            0
        },
        { 
            "COLOR", 
            0, 
            DXGI_FORMAT_R32G32B32A32_FLOAT, // 32bitの浮動小数点数の4つ組
            0, 
            D3D11_APPEND_ALIGNED_ELEMENT, // バッファの読み込みの先頭の場所を示す。
            D3D11_INPUT_PER_VERTEX_DATA, 
            0 
        },
        { 
            "COLOR", 
            1, 
            DXGI_FORMAT_R32G32B32A32_FLOAT, // 32bitの浮動小数点数の4つ組
            0, 
            D3D11_APPEND_ALIGNED_ELEMENT, // バッファの読み込みの先頭の場所を示す。
            D3D11_INPUT_PER_VERTEX_DATA, 
            0 
        }
    };

    // インプットレイアウトが持つ要素の数
    UINT numElements = ARRAYSIZE(layout); //配列の数。つまり頂点シェーダに送るデータ要素の数

    // インプットレイアウトの作成
    HRESULT hr = D3DDevice()->CreateInputLayout
    (
        layout, 
        numElements, 
        compiledShader.get(), // 頂点シェーダーのバイナリデータを指定
        size, 
        &inputLayout
    );

    return hr;
}

HRESULT CreatePixelShader(std::unique_ptr<u8[]>& compiledShader, u32 size, ComPtr<ID3D11PixelShader> &pixelShader)
{
    // ピクセルシェーダの作成
    HRESULT hr = D3DDevice()->CreatePixelShader
    (
        compiledShader.get(),
        size, 
        nullptr, 
        &pixelShader
    );

    return hr;
}

HRESULT CreateWVPMatrix
(
    ComPtr<ID3D11Buffer> &worldMatBuff, 
    ComPtr<ID3D11Buffer> &viewMatBuff, 
    ComPtr<ID3D11Buffer> &projMatBuff, 
    ID3D11DeviceContext* d3dDeviceContext,
    DirectX::XMUINT2 clientSize
){
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(DirectX::XMMATRIX);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;

    HRESULT hr = D3DDevice()->CreateBuffer(&bd, nullptr, &worldMatBuff);
    if (FAILED(hr)) return hr;

    hr = D3DDevice()->CreateBuffer(&bd, nullptr, &viewMatBuff);
    if (FAILED(hr)) return hr;

    hr = D3DDevice()->CreateBuffer(&bd, nullptr, &projMatBuff);
    if (FAILED(hr)) return hr;

    // World
    DirectX::XMMATRIX worldMat = DirectX::XMMatrixIdentity(); //単位行列
    d3dDeviceContext->UpdateSubresource(worldMatBuff.Get(), 0, nullptr, &worldMat, 0, 0);

    // View
    DirectX::XMVECTOR Eye = DirectX::XMVectorSet(0.0f, 0.0f, -6.0f, 0.0f); // 視点（カメラ）座標
    DirectX::XMVECTOR At = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f); // フォーカスする座標
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // カメラの上側を示す単位ベクトル

    DirectX::XMMATRIX ViewMat = DirectX::XMMatrixLookAtLH(Eye, At, Up); // Viewマトリクス作成
    DirectX::XMMATRIX cbView = DirectX::XMMatrixTranspose(ViewMat);

    d3dDeviceContext->UpdateSubresource(viewMatBuff.Get(), 0, nullptr, &cbView, 0, 0);

    // Projection
    DirectX::XMMATRIX projectionMat = DirectX::XMMatrixPerspectiveFovLH
    (
        DirectX::XM_PIDIV4, clientSize.x / (FLOAT)clientSize.y, 0.01f, 10000.0f
    );

    DirectX::XMMATRIX cbProjection = DirectX::XMMatrixTranspose(projectionMat);

    d3dDeviceContext->UpdateSubresource(projMatBuff.Get(), 0, nullptr, &cbProjection, 0, 0);

    return S_OK;
}

HRESULT CreateAffineMatrix(ComPtr<ID3D11Buffer> &affineMatBuff, ID3D11DeviceContext* d3dDeviceContext)
{
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(DirectX::XMMATRIX);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    DirectX::XMMATRIX affineMat = DirectX::XMMatrixIdentity();

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = &affineMat;

    HRESULT hr = D3DDevice()->CreateBuffer(&bd, &initData, &affineMatBuff);
    
    return hr;
}

HRESULT CreateBlendState(ComPtr<ID3D11BlendState> &blendState)
{
    D3D11_BLEND_DESC blendStateDesc;
    blendStateDesc.AlphaToCoverageEnable = FALSE;
    blendStateDesc.IndependentBlendEnable = FALSE;

    // ピクセルの色指定の際に、色の合成が必要な時の処理方法を指定。半透明が描画できる設定になっている
    for (int i = 0; i < 8; i++) { 
        blendStateDesc.RenderTarget[i].BlendEnable = FALSE; // 色合成をしない
        blendStateDesc.RenderTarget[i].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendStateDesc.RenderTarget[i].DestBlend = D3D11_BLEND_INV_SRC_ALPHA; // (1 - ソースアルファ)
        blendStateDesc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD; // 加算
        blendStateDesc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE; // ソースのアルファ値がそのまま使用
        blendStateDesc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO; // ソースとデスティネーションのアルファ成分が加算
        blendStateDesc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD; // 加算
        blendStateDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    }

    blendStateDesc.RenderTarget[0].BlendEnable = TRUE; // １つのレンダリングパスのみ色合成を行う
    HRESULT hr = D3DDevice()->CreateBlendState(&blendStateDesc, &blendState); // 登録

    return hr;
}

HRESULT CreateSamplerState(ComPtr<ID3D11SamplerState> &samplerState)
{
    D3D11_SAMPLER_DESC sampDesc = {};

    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP; // 範囲外の値は最も近い境界値にクランプ（切り落とし）
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0; // 使用する最小ミップマップレベルを指定
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX; // 使用する最大ミップマップレベルを指定

    HRESULT hr = D3DDevice()->CreateSamplerState(&sampDesc, &samplerState);

    return hr;
}

void SetViewPort(DirectX::XMUINT2 clientSize, ID3D11DeviceContext* d3dDeviceContext)
{
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)clientSize.x;
    vp.Height = (FLOAT)clientSize.y;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    d3dDeviceContext->RSSetViewports(1, &vp);
}

HRESULT CreateConstantBuffer(ComPtr<ID3D11Buffer> &constantBuffer)
{
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.ByteWidth = ((sizeof(CBScene) + 15) / 16) * 16;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.MiscFlags = 0;
    cbDesc.StructureByteStride = 0;

    CBScene cbData;
    cbData.colorFilterType = 0;
    cbData.useLightingType = FALSE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = &cbData;

    HRESULT hr = D3DDevice()->CreateBuffer(&cbDesc, nullptr, &constantBuffer);

    return hr;
}

HRESULT CreateCBBloomPramsBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> &cbBloomPramsBuffer, CBBloomPrams& cbData)
{
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.ByteWidth = sizeof(CBBloomPrams);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.MiscFlags = 0;
    cbDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = &cbData;

    HRESULT hr = D3DDevice()->CreateBuffer(&cbDesc, &initData, &cbBloomPramsBuffer);

    return hr;
}

HRESULT UpdateCBBloomPramsBuffer
(
    ID3D11DeviceContext *d3dDeviceContext, ComPtr<ID3D11Buffer> &cbBloomPramsBuffer, CBBloomPrams &cbData
){
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = d3dDeviceContext->Map(cbBloomPramsBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr)) return hr;

    memcpy(mappedResource.pData, &cbData, sizeof(CBBloomPrams));
    d3dDeviceContext->Unmap(cbBloomPramsBuffer.Get(), 0);

    return S_OK;
}

HRESULT CreateObjects(ObjectContainer& container)
{
    HRESULT hr = S_OK;

    // Create triangle sprite
    {
        std::unique_ptr<Vertex[]> vertices = std::make_unique<Vertex[]>(3);
        vertices[0] = 
        { 
            DirectX::XMFLOAT3(-1.6f, -0.9f, 1.0f), 
            DirectX::XMFLOAT2(0.0f, 1.0f), 
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), 
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 
            DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) 
        };
        vertices[1] = 
        { 
            DirectX::XMFLOAT3(1.6f, -0.9f, 1.0f), 
            DirectX::XMFLOAT2(1.0f, 1.0f), 
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), 
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) 
        };
        vertices[2] = 
        { 
            DirectX::XMFLOAT3(1.6f, 0.9f, 1.0f), 
            DirectX::XMFLOAT2(1.0f, 0.0f), 
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), 
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) 
        };

        std::unique_ptr<u32[]> indices = std::make_unique<u32[]>(3);
        indices[0] = 0;
        indices[1] = 2;
        indices[2] = 1;

        ComPtr<ID3D11Buffer> vertexBuffer = CreateVertexBuffer(vertices.get(), 3);
        if (!vertexBuffer) return E_FAIL;

        ComPtr<ID3D11Buffer> indexBuffer = CreateIndexBuffer(indices.get(), 3);
        if (!indexBuffer) return E_FAIL;

        std::unique_ptr<VisualObject> triangle = std::unique_ptr<VisualObject>(new PlainObject
        (
            "White Triangle", false,
            std::move(vertexBuffer), std::move(indexBuffer), 3
        ));

        container.addObject(std::move(triangle));
    }

    // Create red triangle sprite
    {
        std::unique_ptr<Vertex[]> vertices = std::make_unique<Vertex[]>(3);
        vertices[0] = 
        { 
            DirectX::XMFLOAT3(-1.6f, -0.9f, 1.0f), 
            DirectX::XMFLOAT2(0.0f, 1.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),
            DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) 
        };
        vertices[1] = 
        { 
            DirectX::XMFLOAT3(1.6f, -0.9f, 1.0f), 
            DirectX::XMFLOAT2(1.0f, 1.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),
            DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) 
        };
        vertices[2] = 
        { 
            DirectX::XMFLOAT3(1.6f, 0.9f, 1.0f), 
            DirectX::XMFLOAT2(1.0f, 0.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),
            DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) 
        };

        std::unique_ptr<u32[]> indices = std::make_unique<u32[]>(3);
        indices[0] = 0;
        indices[1] = 2;
        indices[2] = 1;

        ComPtr<ID3D11Buffer> vertexBuffer = CreateVertexBuffer(vertices.get(), 3);
        if (!vertexBuffer) return E_FAIL;

        ComPtr<ID3D11Buffer> indexBuffer = CreateIndexBuffer(indices.get(), 3);
        if (!indexBuffer) return E_FAIL;

        std::unique_ptr<VisualObject> redTriangle = std::unique_ptr<VisualObject>(new PlainObject
        (
            "Red Triangle", false,
            std::move(vertexBuffer), std::move(indexBuffer), 3
        ));

        container.addObject(std::move(redTriangle));
    }

    // Create blue square
    {
        std::unique_ptr<Vertex[]> vertices = std::make_unique<Vertex[]>(4);
        vertices[0] = 
        { 
            DirectX::XMFLOAT3(-2.0f, -2.0f, 2.0f), 
            DirectX::XMFLOAT2(0.0f, 1.0f), 
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), 
            DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f)
        };
        vertices[1] = 
        { 
            DirectX::XMFLOAT3(2.0f, -2.0f, 2.0f), 
            DirectX::XMFLOAT2(1.0f, 1.0f), 
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), 
            DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) 
        };
        vertices[2] = 
        { 
            DirectX::XMFLOAT3(2.0f, 2.0f, 2.0f), 
            DirectX::XMFLOAT2(1.0f, 0.0f), 
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), 
            DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f)
        };
        vertices[3] = 
        { 
            DirectX::XMFLOAT3(-2.0f, 2.0f, 2.0f), 
            DirectX::XMFLOAT2(0.0f, 0.0f), 
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), 
            DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f)
        };

        std::unique_ptr<u32[]> indices = std::make_unique<u32[]>(6);
        indices[0] = 1;
        indices[1] = 0;
        indices[2] = 2;
        indices[3] = 0;
        indices[4] = 3;
        indices[5] = 2;

        ComPtr<ID3D11Buffer> vertexBuffer = CreateVertexBuffer(vertices.get(), 4);
        if (!vertexBuffer) return E_FAIL;

        ComPtr<ID3D11Buffer> indexBuffer = CreateIndexBuffer(indices.get(), 6);
        if (!indexBuffer) return E_FAIL;

        std::unique_ptr<VisualObject> square = std::unique_ptr<VisualObject>(new PlainObject
        (
            "Blue Square", false,
            std::move(vertexBuffer), std::move(indexBuffer), 6
        ));

        container.addObject(std::move(square));
    }

    // Create Sprite mini.bmp
    {
        std::unique_ptr<Vertex[]> vertices = std::make_unique<Vertex[]>(4);
        vertices[0] = 
        { 
            DirectX::XMFLOAT3(-1.0f, -1.0f, 1.0f),
            DirectX::XMFLOAT2(0.0f, 1.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
        };
        vertices[1] = 
        { 
            DirectX::XMFLOAT3(1.0f, -1.0f, 1.0f),
            DirectX::XMFLOAT2(1.0f, 1.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
        };
        vertices[2] = 
        { 
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT2(1.0f, 0.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
        };
        vertices[3] = 
        { 
            DirectX::XMFLOAT3(-1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT2(0.0f, 0.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
        };

        std::unique_ptr<u32[]> indices = std::make_unique<u32[]>(6);
        indices[0] = 1;
        indices[1] = 0;
        indices[2] = 2;
        indices[3] = 0;
        indices[4] = 3;
        indices[5] = 2;

        ComPtr<ID3D11Buffer> vertexBuffer = CreateVertexBuffer(vertices.get(), 4);
        if (!vertexBuffer) return E_FAIL;

        ComPtr<ID3D11Buffer> indexBuffer = CreateIndexBuffer(indices.get(), 6);
        if (!indexBuffer) return E_FAIL;

        std::unique_ptr<VisualObject> square = std::unique_ptr<VisualObject>(new TexturedObject
        (
            "Mini.bmp", false, ID_TXT_MINI_BMP,
            std::move(vertexBuffer), std::move(indexBuffer), 6
        ));

        container.addObject(std::move(square));
    }

    // Create Sprite Lenna.tga
    {
        std::unique_ptr<Vertex[]> vertices = std::make_unique<Vertex[]>(4);
        vertices[0] = 
        { 
            DirectX::XMFLOAT3(-1.0f, -1.0f, 1.0f),
            DirectX::XMFLOAT2(0.0f, 1.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
        };
        vertices[1] = 
        { 
            DirectX::XMFLOAT3(1.0f, -1.0f, 1.0f),
            DirectX::XMFLOAT2(1.0f, 1.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
        };
        vertices[2] = 
        { 
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT2(1.0f, 0.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
        };
        vertices[3] = 
        { 
            DirectX::XMFLOAT3(-1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT2(0.0f, 0.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
        };

        std::unique_ptr<u32[]> indices = std::make_unique<u32[]>(6);
        indices[0] = 1;
        indices[1] = 0;
        indices[2] = 2;
        indices[3] = 0;
        indices[4] = 3;
        indices[5] = 2;

        ComPtr<ID3D11Buffer> vertexBuffer = CreateVertexBuffer(vertices.get(), 4);
        if (!vertexBuffer) return E_FAIL;

        ComPtr<ID3D11Buffer> indexBuffer = CreateIndexBuffer(indices.get(), 6);
        if (!indexBuffer) return E_FAIL;

        std::unique_ptr<VisualObject> square = std::unique_ptr<VisualObject>(new TexturedObject
        (
            "Lenna.tga", false, ID_TXT_LENNA_TGA,
            std::move(vertexBuffer), std::move(indexBuffer), 6
        ));

        square->pos_ = DirectX::XMFLOAT3(2.0f, 0.0f, 0.0f);
        square->colorFilter_ = COLOR_FILTER_SEPIA;

        container.addObject(std::move(square));
    }

    // Create Sprite sidaba.dds
    {
        std::unique_ptr<Vertex[]> vertices = std::make_unique<Vertex[]>(4);
        vertices[0] = 
        { 
            DirectX::XMFLOAT3(-1.0f, -1.0f, 1.0f),
            DirectX::XMFLOAT2(0.0f, 1.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
        };
        vertices[1] = 
        { 
            DirectX::XMFLOAT3(1.0f, -1.0f, 1.0f),
            DirectX::XMFLOAT2(1.0f, 1.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
        };
        vertices[2] = 
        { 
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT2(1.0f, 0.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
        };
        vertices[3] = 
        { 
            DirectX::XMFLOAT3(-1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT2(0.0f, 0.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
        };

        std::unique_ptr<u32[]> indices = std::make_unique<u32[]>(6);
        indices[0] = 1;
        indices[1] = 0;
        indices[2] = 2;
        indices[3] = 0;
        indices[4] = 3;
        indices[5] = 2;

        ComPtr<ID3D11Buffer> vertexBuffer = CreateVertexBuffer(vertices.get(), 4);
        if (!vertexBuffer) return E_FAIL;

        ComPtr<ID3D11Buffer> indexBuffer = CreateIndexBuffer(indices.get(), 6);
        if (!indexBuffer) return E_FAIL;

        std::unique_ptr<VisualObject> square = std::unique_ptr<VisualObject>(new TexturedObject
        (
            "Sidaba.dds", false, ID_TXT_SIDABA_DDS,
            std::move(vertexBuffer), std::move(indexBuffer), 6
        ));

        square->pos_ = DirectX::XMFLOAT3(-2.0f, 0.0f, 0.0f);
        square->colorFilter_ = COLOR_FILTER_GRAY_SCALE;

        container.addObject(std::move(square));
    }

    return hr;
}

std::unique_ptr<VisualObject> CreateFullScreenTriangle()
{
    HRESULT hr = S_OK;
    
    std::unique_ptr<Vertex[]> vertices = std::make_unique<Vertex[]>(4);
    vertices[0] = 
    { 
        DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f),
        DirectX::XMFLOAT2(0.0f, 1.0f),
        DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
        DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
        DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
    };

    vertices[1] = 
    { 
        DirectX::XMFLOAT3(-1.0f, 3.0f, 0.0f),
        DirectX::XMFLOAT2(0.0f, -1.0f),
        DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
        DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
        DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
    };

    vertices[2] = 
    { 
        DirectX::XMFLOAT3(3.0f, -1.0f, 0.0f),
        DirectX::XMFLOAT2(2.0f, 1.0f),
        DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
        DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
        DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
    };

    std::unique_ptr<u32[]> indices = std::make_unique<u32[]>(3);
    indices[0] = 1;
    indices[1] = 2;
    indices[2] = 0;

    ComPtr<ID3D11Buffer> vertexBuffer = CreateVertexBuffer(vertices.get(), 3);
    if (!vertexBuffer) return nullptr;

    ComPtr<ID3D11Buffer> indexBuffer = CreateIndexBuffer(indices.get(), 3);
    if (!indexBuffer) return nullptr;

    std::unique_ptr<VisualObject> triangle = std::unique_ptr<VisualObject>(new PlainObject
    (
        "Full Screen Triangle", true,
        std::move(vertexBuffer), std::move(indexBuffer), 6
    ));

    return triangle;
}

DirectX::XMFLOAT3 GetFloat3FromChar(u8*& start, u8* end, char divideChar, char endChar)
{
    float val[3] = { 0.0f, 0.0f, 0.0f };
    u32 stored = 0;
    u32 size = 0;

    while (true)
    {
        if (*(start + size) != divideChar && *(start + size) != endChar && start + size != end)
        {
            size++;
            continue;
        }

        if (stored != 3)
        {
            std::string str(start, start + size);
            val[stored] = std::stof(str);
            stored++;
        }

        start += size + 1;
        size = 0;

        if (stored == 3 && *(start - 1) == endChar) break;
    }

    return {val[0], val[1], val[2]};
}

DirectX::XMINT3 GetInt3FromChar(u8*& start, u8* end, char divideChar, char endChar)
{
    int val[3] = { 0, 0, 0 };
    u32 stored = 0;
    u32 size = 0;

    while (true)
    {
        if (*(start + size) != divideChar && *(start + size) != endChar && start + size != end)
        {
            size++;
            continue;
        }

        if (stored != 3)
        {
            std::string str(start, start + size);
            val[stored] = std::stof(str);
            stored++;
        }

        start += size + 1;
        size = 0;

        if (stored == 3 && *(start - 1) == endChar) break;
    }

    return {val[0], val[1], val[2]};
}

DirectX::XMFLOAT2 GetFloat2FromChar(u8*& start, u8* end, char divideChar, char endChar)
{
    float val[2] = { 0.0f, 0.0f};
    u32 stored = 0;
    u32 size = 0;

    while (true)
    {
        if (*(start + size) != divideChar && *(start + size) != endChar && start + size != end)
        {
            size++;
            continue;
        }

        if (stored != 2)
        {
            std::string str(start, start + size);
            val[stored] = std::stof(str);
            stored++;
        }

        start += size + 1;
        size = 0;

        if (stored == 2 && *(start - 1) == endChar) break;
    }

    return {val[0], val[1]};
}

std::array<MemRange, 3> GetMemRange3FromChar(u8*& start, u8* end, char divideChar, char endChar)
{
    std::array<MemRange, 3> memRanges;
    u32 stored = 0;
    u32 size = 0;

    while (true)
    {
        if (*(start + size) != divideChar && *(start + size) != endChar && start + size != end)
        {
            size++;
            continue;
        }

        if (stored != 3)
        {
            memRanges[stored].start = start;
            memRanges[stored].end = start + size;
            stored++;
        }

        start += size + 1;
        size = 0;

        if (stored == 3 && *(start - 1) == endChar) break;
    }

    return memRanges;
}

HRESULT CreateObjectFromObjFile
(
    std::unique_ptr<u8[]>& buff, fpos_t& size, 
    std::string name, bool needsDisplayed, LIGHTING_MODE lightingMode,
    ObjectContainer& container
){
    std::vector<DirectX::XMFLOAT3> srcVertices;
    std::vector<DirectX::XMFLOAT2> srcUvCoords;
    std::vector<DirectX::XMFLOAT3> srcNormals;

    // verticesとindicesのデータとなる
    std::vector<Vertex> dstVertices;
    std::vector<u32> dstIndices;

    DirectX::XMFLOAT4 col = { 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT4 colScale = { 0.0f, 0.0f, 0.0f, 0.0f };

    // 頂点座標、UV座標の組み合わせが同じものが複数存在しないようにするためのマップ
    std::unordered_map<std::tuple<u32, u32>, u32, TupleHash> vertexIdMap;

    u8* ptr = reinterpret_cast<u8*>(buff.get());
    u8* end = ptr + size;

    while (ptr < end)
    {
        if (*(ptr - 1) == '\n' && *ptr == 'v' && *(ptr + 1) == ' ')
        {
            ptr += 3;
            srcVertices.emplace_back(GetFloat3FromChar(ptr, end, ' ', '\n'));
            srcVertices.back().z *= -1.0f; // 座標系の変換
        }
        else if (*(ptr - 1) == '\n' && *ptr == 'v' && *(ptr + 1) == 't')
        {
            ptr += 3;
            srcUvCoords.emplace_back(GetFloat2FromChar(ptr, end, ' ', '\n'));
            srcUvCoords.back().y = 1.0f - srcUvCoords.back().y; // 座標系の変換
        }
        else if (*(ptr - 1) == '\n' && *ptr == 'v' && *(ptr + 1) == 'n')
        {
            ptr += 3;
            srcNormals.emplace_back(GetFloat3FromChar(ptr, end, ' ', '\n'));
            srcNormals.back().z *= -1.0f; // 座標系の変換
        }
        else if (*(ptr - 1) == '\n' && *ptr == 'f' && *(ptr + 1) == ' ')
        {
            ptr += 2;
            std::array<MemRange, 3> memRanges = GetMemRange3FromChar(ptr, end, ' ', '\n');

            DirectX::XMINT3 v1 = GetInt3FromChar(memRanges[0].start, end, '/', ' ');
            DirectX::XMINT3 v2 = GetInt3FromChar(memRanges[1].start, end, '/', ' ');
            DirectX::XMINT3 v3 = GetInt3FromChar(memRanges[2].start, end, '/', '\n');

            if (vertexIdMap.try_emplace(std::make_tuple(v1.x, v1.y), dstVertices.size()).second)
            {
                dstVertices.emplace_back
                (
                    srcVertices[v1.x - 1], srcUvCoords[v1.y - 1], srcNormals[v1.z - 1],
                    col, colScale
                );
            }

            if (vertexIdMap.try_emplace(std::make_tuple(v2.x, v2.y), dstVertices.size()).second)
            {
                dstVertices.emplace_back
                (
                    srcVertices[v2.x - 1], srcUvCoords[v2.y - 1], srcNormals[v2.z - 1],
                    col, colScale
                );
            }

            if (vertexIdMap.try_emplace(std::make_tuple(v3.x, v3.y), dstVertices.size()).second)
            {
                dstVertices.emplace_back
                (
                    srcVertices[v3.x - 1], srcUvCoords[v3.y - 1], srcNormals[v3.z - 1],
                    col, colScale
                );
            }

            dstIndices.emplace_back(vertexIdMap[std::make_tuple(v1.x, v1.y)]);
            dstIndices.emplace_back(vertexIdMap[std::make_tuple(v3.x, v3.y)]);
            dstIndices.emplace_back(vertexIdMap[std::make_tuple(v2.x, v2.y)]);
        }
        else
        {
            ptr++;
        }
    }
    
    ComPtr<ID3D11Buffer> vertexBuffer = CreateVertexBuffer(dstVertices.data(), dstVertices.size());
    if (!vertexBuffer) return E_FAIL;

    ComPtr<ID3D11Buffer> indexBuffer = CreateIndexBuffer(dstIndices.data(), dstIndices.size());
    if (!indexBuffer) return E_FAIL;

    std::unique_ptr<VisualObject> object = std::make_unique<PlainObject>
    (
        name, needsDisplayed,
        std::move(vertexBuffer), std::move(indexBuffer), dstIndices.size()
    );

    object->useLightingMode_ = lightingMode;

    container.addObject(std::move(object));

    return S_OK;
}

HRESULT ShowDebugWindow
(
    ID3D11DeviceContext* d3dDeviceContext,
    ObjectContainer& objectContainer, int &selectingObjId,
    LightContainer& lightContainer, int& selectingLightId,
    bool& usingBloom, ComPtr<ID3D11Buffer> cbBloomPramsBuffer, CBBloomPrams& bloomParams
){
    HRESULT hr = S_OK;

    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Debug Window"))
    {
        if (ImGui::Button("Create Stanford Bunny"))
        {
            fpos_t size;
            std::unique_ptr<u8[]> buff = LoadFile("objects/bunny.obj", size);

            hr = CreateObjectFromObjFile(buff, size, "Stanford Bunny", true, LIGHTING_PHONG, objectContainer);
            if (FAILED(hr)) return hr;
        }

        ImGui::BeginTable("Object List", 6, ImGuiTableFlags_SizingFixedFit);
        for (u32 i = 0; i < objectContainer.getContainerSize(); ++i)
        {
            VisualObject* object = objectContainer.getData(i);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("Object ID[%d]: %s", i, object->getName().c_str());

            ImGui::TableNextColumn();
            bool displayFlag = object->needsDisplayed_;
            if (ImGui::Checkbox(("Show : " + std::to_string(i)).c_str(), &displayFlag)) object->needsDisplayed_ = displayFlag;

            ImGui::TableNextColumn();
            bool sepiaFlag = object->colorFilter_ == COLOR_FILTER_SEPIA;
            if (ImGui::Checkbox(("Sepia : " + std::to_string(i)).c_str(), &sepiaFlag)) 
            {
                if (sepiaFlag) object->colorFilter_ = COLOR_FILTER_SEPIA;
            }

            ImGui::TableNextColumn();
            bool grayScaleFlag = object->colorFilter_ == COLOR_FILTER_GRAY_SCALE;
            if (ImGui::Checkbox(("Gray Scale : " + std::to_string(i)).c_str(), &grayScaleFlag)) 
            {
                if (grayScaleFlag) object->colorFilter_ = COLOR_FILTER_GRAY_SCALE;
            }

            if (!sepiaFlag && !grayScaleFlag) object->colorFilter_ = COLOR_FILTER_NONE;

            ImGui::TableNextColumn();
            bool normalLightingFlag = object->useLightingMode_ == LIGHTING_NORMAL_BASE;
            if (ImGui::Checkbox(("Normal Base Lighting : " + std::to_string(i)).c_str(), &normalLightingFlag)) 
            {
                if (normalLightingFlag) object->useLightingMode_ = LIGHTING_NORMAL_BASE;
            }

            ImGui::TableNextColumn();
            bool phongLightingFlag = object->useLightingMode_ == LIGHTING_PHONG;
            if (ImGui::Checkbox(("Phong Lighting : " + std::to_string(i)).c_str(), &phongLightingFlag)) 
            {
                if (phongLightingFlag) object->useLightingMode_ = LIGHTING_PHONG;
            }

            if (!normalLightingFlag && !phongLightingFlag) object->useLightingMode_ = LIGHTING_NONE;
        }
        ImGui::EndTable();

        ImGui::Dummy(ImVec2(0.0f, 20.0f));  

        VisualObject* selectingObj = objectContainer.getData(selectingObjId);
        u32 containerSize = objectContainer.getContainerSize();
        
        ImGui::Text("Selecting Object: %s",  selectingObj->getName().c_str());
        if (ImGui::InputInt("Selecting Object ID", &selectingObjId)) 
        {
            if (selectingObjId < 0) selectingObjId = 0;
            else if (selectingObjId >= containerSize) selectingObjId = containerSize - 1;
        }

        ImGui::Dummy(ImVec2(0.0f, 20.0f));

        ImGui::BeginTable("Selecting Object parameters", 3, ImGuiTableFlags_SizingFixedFit);
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::SliderFloat("Position X", &selectingObj->pos_.x, -5.0f, 5.0f);
        ImGui::SliderFloat("Position Y", &selectingObj->pos_.y, -5.0f, 5.0f);
        ImGui::SliderFloat("Position Z", &selectingObj->pos_.z, -5.0f, 5.0f);

        ImGui::TableNextColumn();
        ImGui::SliderFloat("Scale X", &selectingObj->scale_.x, 0.1f, 5.0f);
        ImGui::SliderFloat("Scale Y", &selectingObj->scale_.y, 0.1f, 5.0f);
        ImGui::SliderFloat("Scale Z", &selectingObj->scale_.z, 0.1f, 5.0f);

        ImGui::TableNextColumn();
        ImGui::SliderFloat("Rotate X", &selectingObj->rotate_.x, -6.28f, 6.28f);
        ImGui::SliderFloat("Rotate Y", &selectingObj->rotate_.y, -6.28f, 6.28f);
        ImGui::SliderFloat("Rotate Z", &selectingObj->rotate_.z, -6.28f, 6.28f);

        ImGui::EndTable();

        ImGui::Dummy(ImVec2(0.0f, 20.0f));

        if (ImGui::InputInt("Selecting Light ID", &selectingLightId)) 
        {
            if (selectingLightId < 0) selectingLightId = 0;
            else if (selectingLightId >= lightContainer.getContainerSize())
            {
                selectingLightId = lightContainer.getContainerSize() - 1;
            }
        }

        ImGui::BeginTable("Selecting Light parameters", 6, ImGuiTableFlags_SizingFixedFit);
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::Dummy(ImVec2(0.0f, 20.0f));

        LightData* selectingLight = lightContainer.getData(selectingLightId);

        ImGui::Text("Light ID[%d]", selectingLightId);

        bool lightParamUpdate = false;

        ImGui::TableNextColumn();
        lightParamUpdate |= ImGui::SliderFloat("Light Pos X", &selectingLight->srcBuffer.LightPos.x, -20.0f, 20.0f);
        lightParamUpdate |= ImGui::SliderFloat("Light Pos Y", &selectingLight->srcBuffer.LightPos.y, -20.0f, 20.0f);
        lightParamUpdate |= ImGui::SliderFloat("Light Pos Z", &selectingLight->srcBuffer.LightPos.z, -20.0f, 20.0f);

        ImGui::TableNextColumn();
        lightParamUpdate |= ImGui::SliderFloat("Light Color R", &selectingLight->srcBuffer.LightColor.x, 0.0f, 1.0f);
        lightParamUpdate |= ImGui::SliderFloat("Light Color G", &selectingLight->srcBuffer.LightColor.y, 0.0f, 1.0f);
        lightParamUpdate |= ImGui::SliderFloat("Light Color B", &selectingLight->srcBuffer.LightColor.z, 0.0f, 1.0f);

        ImGui::TableNextColumn();
        lightParamUpdate |= ImGui::SliderFloat("View Pos X", &selectingLight->srcBuffer.ViewPos.x, -50.0f, 50.0f);
        lightParamUpdate |= ImGui::SliderFloat("View Pos Y", &selectingLight->srcBuffer.ViewPos.y, -50.0f, 50.0f);
        lightParamUpdate |= ImGui::SliderFloat("View Pos Z", &selectingLight->srcBuffer.ViewPos.z, -50.0f, 50.0f);

        ImGui::TableNextColumn();
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        lightParamUpdate |= ImGui::SliderFloat("Ambient Strength", &selectingLight->srcBuffer.AmbientStrength, 0.0f, 1.0f);

        ImGui::TableNextColumn();
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        lightParamUpdate |= ImGui::SliderInt("Glossiness", &selectingLight->srcBuffer.Glossiness, 1, 256);

        ImGui::EndTable();

        if (lightParamUpdate)
        {
            UpdateLightBuffer
            (
                d3dDeviceContext, 
                selectingLight->buffer, 
                selectingLight->srcBuffer
            );
        }

        ImGui::Dummy(ImVec2(0.0f, 20.0f));

        bool bloomParamUpdate = false;

        ImGui::BeginTable("Bloom parameters", 4, ImGuiTableFlags_SizingFixedFit);
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::Checkbox("Use Bloom", &usingBloom);

        ImGui::TableNextColumn();
        bloomParamUpdate |= ImGui::SliderFloat("Threshold", &bloomParams.threshold, 0.0f, 1.0f);

        ImGui::TableNextColumn();
        bloomParamUpdate |= ImGui::SliderFloat("Intensity", &bloomParams.intensity, 0.0f, 10.0f);

        ImGui::TableNextColumn();
        bloomParamUpdate |= ImGui::SliderFloat("Blur Scale", &bloomParams.blurScale, 0.0f, 10.0f);

        ImGui::EndTable();

        if (bloomParamUpdate)
        {
            UpdateCBBloomPramsBuffer(d3dDeviceContext, cbBloomPramsBuffer, bloomParams);
        }
    }

    ImGui::End();

    return hr;
}

ComPtr<ID3D11Buffer> CreateVertexBuffer(Vertex *vertices, u32 vertexSize)
{
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Vertex) * vertexSize;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0; 
    bd.MiscFlags = 0;
    bd.StructureByteStride = sizeof(Vertex);
    D3D11_SUBRESOURCE_DATA InitData = {};

    InitData.pSysMem = vertices;

    ComPtr<ID3D11Buffer> vertexBuff;
    HRESULT hr = D3DDevice()->CreateBuffer
    (
        &bd,
        &InitData,
        &vertexBuff
    );

    if (FAILED(hr)) return nullptr;

    return vertexBuff;
}

ComPtr<ID3D11Buffer> CreateIndexBuffer(u32 *indices, u32 indexSize)
{
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(u32) * indexSize;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0; 
    bd.MiscFlags = 0;
    bd.StructureByteStride = sizeof(u32);
    D3D11_SUBRESOURCE_DATA InitData = {};

    InitData.pSysMem = indices;

    ComPtr<ID3D11Buffer> indexBuff;
    HRESULT hr = D3DDevice()->CreateBuffer
    (
        &bd,
        &InitData,
        &indexBuff
    );

    if (FAILED(hr)) return nullptr;

    return indexBuff;
}

HRESULT CreateTextureBuffer
(
    ComPtr<ID3D11Texture2D> &texture, 
    ComPtr<ID3D11ShaderResourceView> &view, 
    DirectX::XMUINT2 clientSize, std::unique_ptr<u8[]> &pixels
){
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = clientSize.x;
    desc.Height = clientSize.y;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    // サブリソースデータ
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = pixels.get();
    initData.SysMemPitch = clientSize.x * 4; // 1ピクセルあたり4バイト（BGRA）

    // テクスチャ作成
    HRESULT hr = D3DDevice()->CreateTexture2D(&desc, &initData, &texture);
    if (FAILED(hr)) return hr;

    // シェーダーリソースビューの説明
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    // シェーダーリソースビュー作成
    hr = D3DDevice()->CreateShaderResourceView(texture.Get(), &srvDesc, &view);
    if (FAILED(hr)) return hr;
}

HRESULT CreateTextures(Converter &converter, TextureContainer &container)
{
    HRESULT hr = S_OK;

    container.addTexture("dummy"); // ID 0
    container.addTexture("images/mini.bmp"); // ID 1
    container.addTexture("images/Lenna.tga"); // ID 2
    container.addTexture("images/sidaba.dds"); // ID 3

    // Create dummy texture
    {
        std::unique_ptr<u8[]> dummyPixels = std::make_unique<u8[]>(4);
        dummyPixels[0] = 255;
        dummyPixels[1] = 255;
        dummyPixels[2] = 255;
        dummyPixels[3] = 255;

        TextureData* texture = container.getTexture(0);
        if (texture == nullptr) return 1;

        hr = CreateTextureBuffer(texture->texture, texture->view, DirectX::XMUINT2(1, 1), dummyPixels);
        if (FAILED(hr)) return hr;
    }   

    // Create other textures
    for (u32 i = 1; i < container.getContainerSize(); ++i)
    {
        TextureData* texture = container.getTexture(i);
        if (texture == nullptr) return 1;

        std::unique_ptr<FileData> fileData = converter.fileAnalysis(texture->path);
        if (fileData == nullptr) return 1;

        PixelFlipper flipper;
        flipper.getFlipTypeToTLBR(PixelStorageOrder::bottomLeftToTopRight); // FileDataはBLTRなのでTLBRに変換
        std::unique_ptr<u8[]> srcPixels = std::make_unique<u8[]>(fileData->width * fileData->height * 4);
        flipper.insertPixelsFlippedRGBA(srcPixels, 0, fileData->pixels, fileData->width, fileData->height);

        hr = CreateTextureBuffer
        (
            texture->texture, texture->view, 
            DirectX::XMUINT2(fileData->width, fileData->height), srcPixels
        );
        if (FAILED(hr)) return hr;
    }

    return hr;
}

HRESULT CreateLights(LightContainer &container)
{
    {
        std::unique_ptr<LightData> light = std::make_unique<LightData>();

        light->srcBuffer.LightPos = DirectX::XMFLOAT3(3.0f, 3.0f, 0.0f);
        light->srcBuffer.LightColor = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
        light->srcBuffer.ViewPos = DirectX::XMFLOAT3(0.0f, 0.0f, 10.0f);
        light->srcBuffer.AmbientStrength = 0.1f;
        light->srcBuffer.Glossiness = 32;

        HRESULT hr = CreateLightBuffer(light->buffer, light->srcBuffer);
        if (FAILED(hr)) return hr;

        container.addLight(std::move(light));
    }

    return S_OK;
}

HRESULT CreateLightBuffer(ComPtr<ID3D11Buffer> &lightBuffer, LightBuffer& lightData)
{
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.ByteWidth = sizeof(LightBuffer);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.MiscFlags = 0;
    cbDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = &lightData;

    HRESULT hr = D3DDevice()->CreateBuffer(&cbDesc, &initData, &lightBuffer);

    return hr;
}

void SetLightBuffer(ID3D11DeviceContext *d3dDeviceContext, ComPtr<ID3D11Buffer> &lightBuffer)
{
    d3dDeviceContext->PSSetConstantBuffers(4, 1, lightBuffer.GetAddressOf());
}

void UpdateLightBuffer
(
    ID3D11DeviceContext *d3dDeviceContext, 
    ComPtr<ID3D11Buffer> &lightBuffer, LightBuffer &lightData
){
    D3D11_MAPPED_SUBRESOURCE cbMappedResource = { 0 };
    d3dDeviceContext->Map(lightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cbMappedResource);
    memcpy_s(cbMappedResource.pData, sizeof(LightBuffer), &lightData, sizeof(LightBuffer));
    d3dDeviceContext->Unmap(lightBuffer.Get(), 0);
}

HRESULT CreateRenderTarget(DirectX::XMUINT2 clientSize, RenderTarget& renderTarget)
{
    // テクスチャ作成
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = clientSize.x;
    textureDesc.Height = clientSize.y;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = D3DDevice()->CreateTexture2D(&textureDesc, nullptr, renderTarget.texture.GetAddressOf());
    if (FAILED(hr)) return hr;

    // RenderTargetView作成
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    hr = D3DDevice()->CreateRenderTargetView(renderTarget.texture.Get(), &rtvDesc, renderTarget.view.GetAddressOf());
    if (FAILED(hr)) return hr;

    // ShaderResourceView作成
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    hr = D3DDevice()->CreateShaderResourceView(renderTarget.texture.Get(), &srvDesc, renderTarget.srv.GetAddressOf());
    if (FAILED(hr)) return hr;
    
    return hr;
}
