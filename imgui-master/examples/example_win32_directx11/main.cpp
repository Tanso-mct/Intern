#include "pch.h"

// Dear ImGui: standalone example application for DirectX 11

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <d3d11.h>
#include <iostream>
#include <vector>
#include <map>

#include "helpers.h"
#include "slot_indicies.h"
#include "visual_object.h"
#include "texture.h"
#include "light.h"

#include "converter.h"
#include "format_bmp.h"
#include "format_tga.h"
#include "format_dds.h"

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#include <DirectXMath.h>

// Data
static ID3D11DeviceContext* gD3dDeviceContext = nullptr;
static IDXGISwapChain* gSwapChain = nullptr;
static bool gSwapChainOccluded = false;
static u32 gResizeWidth = 0, gResizeHeight = 0;
static ID3D11RenderTargetView* gMainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, u32 msg, WPARAM wParam, LPARAM lParam);

// Main code
int main(int argc, char* argv[])
{
    /*************************************************************************************************************** */
    // Create application window
    /*************************************************************************************************************** */
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = 
    { 
        sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, 
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr 
    };

    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowW
    (
        wc.lpszClassName, L"Dear ImGui DirectX11 Example", 
        WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr
    );

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    HRESULT hr = S_OK;

    /*************************************************************************************************************** */
    // Initialize Image Format Converter
    /*************************************************************************************************************** */
    // Converter
    Converter converter;
    converter.addObserver("bmp", std::make_unique<BMP>());
    converter.addObserver("tga", std::make_unique<TGA>(true));
    converter.addObserver("dds", std::make_unique<DDS>());

    /*************************************************************************************************************** */
    // Initialize Direct3D
    /*************************************************************************************************************** */
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    
    DirectX::XMUINT2 clientSize = GetClientSize(hwnd);

    // Create Depth Stencil
    ComPtr<ID3D11Texture2D> depthStencil = nullptr;
    ComPtr<ID3D11DepthStencilView> depthStencilView = nullptr;
    hr = CreateDepthStencil(depthStencil, depthStencilView, clientSize);
    if (FAILED(hr)) return 1;
    
    // Create World, View, Projection Matrix
    ComPtr<ID3D11Buffer> worldMatBuff = nullptr;
    ComPtr<ID3D11Buffer> viewMatBuff = nullptr;
    ComPtr<ID3D11Buffer> projMatBuff = nullptr;
    hr = CreateWVPMatrix(worldMatBuff, viewMatBuff, projMatBuff, gD3dDeviceContext, clientSize);
    if (FAILED(hr)) return 1;

    // Create affine matrix
    ComPtr<ID3D11Buffer> affineMatBuff = nullptr;
    hr = CreateAffineMatrix(affineMatBuff, gD3dDeviceContext);
    if (FAILED(hr)) return 1;

    // Create constant buffer for pixel shader
    ComPtr<ID3D11Buffer> constantBuffer = nullptr;
    hr = CreateConstantBuffer(constantBuffer);
    if (FAILED(hr)) return 1;

    // Create constant buffer for bloom parameters
    ComPtr<ID3D11Buffer> cbBloomPramsBuffer = nullptr;
    CBBloomPrams bloomParams;
    {
        bloomParams.threshold = 0.8f;
        bloomParams.texSize = DirectX::XMFLOAT2(1 / (float)clientSize.x, 1 / (float)clientSize.y);
        bloomParams.intensity = 1.0f;
        bloomParams.blurScale = 3.0f;
    }

    hr = CreateCBBloomPramsBuffer(cbBloomPramsBuffer, bloomParams);
    if (FAILED(hr)) return 1;

    // Create Blend State
    ComPtr<ID3D11BlendState> blendState = nullptr;
    hr = CreateBlendState(blendState);
    if (FAILED(hr)) return 1;

    // Create Sampler State
    ComPtr<ID3D11SamplerState> samplerState = nullptr;
    hr = CreateSamplerState(samplerState);
    if (FAILED(hr)) return 1;

    /*************************************************************************************************************** */
    // Render Targets and Shaders
    /*************************************************************************************************************** */
    RenderTarget sceneRT;
    hr = CreateRenderTarget(clientSize, sceneRT);
    if (FAILED(hr)) return 1;

    // Create Shader and Input Layout
    ComPtr<ID3D11VertexShader> sceneVS = nullptr;
    ComPtr<ID3D11PixelShader> scenePS = nullptr;
    ComPtr<ID3D11InputLayout> sceneIL = nullptr;  
    {
        fpos_t compiledVSSize = 0;
        std::unique_ptr<u8[]> compiledVS = LoadFile("data/SceneVS.cso", compiledVSSize);
        hr = CreateVertexShader(compiledVS, compiledVSSize, sceneVS);
        if (FAILED(hr)) return 1;

        fpos_t compiledPSSize = 0;
        std::unique_ptr<u8[]> compiledPS = LoadFile("data/ScenePS.cso", compiledPSSize);
        hr = CreatePixelShader(compiledPS, compiledPSSize, scenePS);
        if (FAILED(hr)) return 1;

        hr = CreateInputLayout(compiledVS, compiledVSSize, sceneIL);
        if (FAILED(hr)) return 1;
    }

    RenderTarget bloomRT;
    hr = CreateRenderTarget(clientSize, bloomRT);
    if (FAILED(hr)) return 1;
    ComPtr<ID3D11PixelShader> bloomPS = nullptr;
    {
        fpos_t bloomPSSize = 0;
        std::unique_ptr<u8[]> compiledBloomPS = LoadFile("data/BloomPS.cso", bloomPSSize);
        hr = CreatePixelShader(compiledBloomPS, bloomPSSize, bloomPS);
        if (FAILED(hr)) return 1;
    }

    RenderTarget horizBlurRT;
    hr = CreateRenderTarget(clientSize, horizBlurRT);
    if (FAILED(hr)) return 1;
    ComPtr<ID3D11PixelShader> horizBlurPS = nullptr;
    {
        fpos_t horizBlurPSSize = 0;
        std::unique_ptr<u8[]> compiledHorizBlurPS = LoadFile("data/HorizBlurPS.cso", horizBlurPSSize);
        hr = CreatePixelShader(compiledHorizBlurPS, horizBlurPSSize, horizBlurPS);
        if (FAILED(hr)) return 1;
    }

    RenderTarget vertBlurRT;
    hr = CreateRenderTarget(clientSize, vertBlurRT);
    if (FAILED(hr)) return 1;
    ComPtr<ID3D11PixelShader> vertBlurPS = nullptr;
    {
        fpos_t vertBlurPSSize = 0;
        std::unique_ptr<u8[]> compiledVertBlurPS = LoadFile("data/VertBlurPS.cso", vertBlurPSSize);
        hr = CreatePixelShader(compiledVertBlurPS, vertBlurPSSize, vertBlurPS);
        if (FAILED(hr)) return 1;
    }

    RenderTarget combineRT;
    hr = CreateRenderTarget(clientSize, combineRT);
    if (FAILED(hr)) return 1;
    ComPtr<ID3D11PixelShader> combinePS = nullptr;
    {
        fpos_t combinePSSize = 0;
        std::unique_ptr<u8[]> compiledCombinePS = LoadFile("data/CombinePS.cso", combinePSSize);
        hr = CreatePixelShader(compiledCombinePS, combinePSSize, combinePS);
        if (FAILED(hr)) return 1;
    }

    ComPtr<ID3D11VertexShader> FullScreenQuadVS = nullptr;
    ComPtr<ID3D11PixelShader> FullScreenQuadPS = nullptr;
    {
        fpos_t outputVSSize = 0;
        std::unique_ptr<u8[]> compiledOutputVS = LoadFile("data/FullScreenQuadVS.cso", outputVSSize);
        hr = CreateVertexShader(compiledOutputVS, outputVSSize, FullScreenQuadVS);
        if (FAILED(hr)) return 1;

        fpos_t outputPSSize = 0;
        std::unique_ptr<u8[]> compiledOutputPS = LoadFile("data/FullScreenQuadPS.cso", outputPSSize);
        hr = CreatePixelShader(compiledOutputPS, outputPSSize, FullScreenQuadPS);
        if (FAILED(hr)) return 1;
    }

    /*************************************************************************************************************** */
    // Create Texture
    /*************************************************************************************************************** */
    TextureContainer textureContainer;
    hr = CreateTextures(converter, textureContainer);
    if (FAILED(hr)) return 1;

    /*************************************************************************************************************** */
    // Create Object
    /*************************************************************************************************************** */
    ObjectContainer objectContainer;
    hr = CreateObjects(objectContainer);
    if (FAILED(hr)) return 1;

    /*************************************************************************************************************** */
    // Create Full Screen Triangle
    /*************************************************************************************************************** */
    std::unique_ptr<VisualObject> fsqTriangleObj = CreateFullScreenTriangle();
    if (!fsqTriangleObj) return 1;

    /*************************************************************************************************************** */
    // Create Light
    /*************************************************************************************************************** */
    LightContainer lightContainer;
    hr = CreateLights(lightContainer);
    if (FAILED(hr)) return 1;

    /*************************************************************************************************************** */
    // ImGui
    /*************************************************************************************************************** */
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(D3DDevice().Get(), gD3dDeviceContext);

    // Our state
    bool showDemoWindow = false;
    bool showDebugWindow = true;
    ImVec4 clearColorBlack = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
    ImVec4 clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    const float clearColorWithAlpha[4] = 
    {
        clearColor.x * clearColor.w, 
        clearColor.y * clearColor.w, 
        clearColor.z * clearColor.w, 
        clearColor.w 
    };

    const float clearColorBlackWithAlpha[4] = 
    {
        clearColorBlack.x * clearColorBlack.w, 
        clearColorBlack.y * clearColorBlack.w, 
        clearColorBlack.z * clearColorBlack.w, 
        clearColorBlack.w 
    };

    io.FontGlobalScale = 1.0f;

    /*************************************************************************************************************** */
    // Main loop
    /*************************************************************************************************************** */
    bool done = false;
    int selectingObjId = 0;
    int selectingLightId = 0;
    bool usingBloom = true;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (gSwapChainOccluded && gSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        gSwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (gResizeWidth != 0 && gResizeHeight != 0)
        {
            depthStencil = nullptr;
            depthStencilView = nullptr;
            CleanupRenderTarget();

            clientSize = GetClientSize(hwnd);

            gSwapChain->ResizeBuffers(0, clientSize.x, clientSize.y, DXGI_FORMAT_UNKNOWN, 0);
            gResizeWidth = gResizeHeight = 0;

            CreateRenderTarget();

            hr = CreateRenderTarget(clientSize, sceneRT);
            if (FAILED(hr)) return 1;

            hr = CreateRenderTarget(clientSize, bloomRT);
            if (FAILED(hr)) return 1;

            hr = CreateRenderTarget(clientSize, horizBlurRT);
            if (FAILED(hr)) return 1;

            hr = CreateRenderTarget(clientSize, vertBlurRT);
            if (FAILED(hr)) return 1;

            hr = CreateRenderTarget(clientSize, combineRT);
            if (FAILED(hr)) return 1;

            CreateDepthStencil(depthStencil, depthStencilView, clientSize);

            bloomParams.texSize = DirectX::XMFLOAT2((float)clientSize.x, (float)clientSize.y);
            hr = UpdateCBBloomPramsBuffer(gD3dDeviceContext, cbBloomPramsBuffer, bloomParams);
            if (FAILED(hr)) return 1;
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // ImGui windows
        if (showDemoWindow) ImGui::ShowDemoWindow(&showDemoWindow);
        if (showDebugWindow)
        {
            hr = ShowDebugWindow
            (
                gD3dDeviceContext, 
                objectContainer, selectingObjId, 
                lightContainer, selectingLightId,
                usingBloom, cbBloomPramsBuffer, bloomParams
            );
            if (FAILED(hr)) return 1;
        }

        // Rendering
        ImGui::Render();

        SetViewPort(clientSize, gD3dDeviceContext);
        gD3dDeviceContext->IASetInputLayout(sceneIL.Get());
        gD3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        gD3dDeviceContext->VSSetConstantBuffers(0, 1, viewMatBuff.GetAddressOf());
        gD3dDeviceContext->VSSetConstantBuffers(1, 1, projMatBuff.GetAddressOf());
        gD3dDeviceContext->VSSetConstantBuffers(2, 1, affineMatBuff.GetAddressOf());
        gD3dDeviceContext->PSSetConstantBuffers(3, 1, constantBuffer.GetAddressOf());
        gD3dDeviceContext->PSSetConstantBuffers(4, 1, lightContainer.getData(ID_LIGHT_POINT)->buffer.GetAddressOf());
        gD3dDeviceContext->PSSetConstantBuffers(5, 1, cbBloomPramsBuffer.GetAddressOf());

        gD3dDeviceContext->ClearDepthStencilView
        (
            depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0
        );

        if (usingBloom)
        {
            // Scene
            gD3dDeviceContext->OMSetRenderTargets(1, sceneRT.view.GetAddressOf(), depthStencilView.Get());
            gD3dDeviceContext->ClearRenderTargetView(sceneRT.view.Get(), clearColorWithAlpha);
            gD3dDeviceContext->VSSetShader(sceneVS.Get(), nullptr, 0);
            gD3dDeviceContext->PSSetShader(scenePS.Get(), nullptr, 0);

            // Draw objects and execute vertex shader
            for (u32 i = 0; i < objectContainer.getContainerSize(); ++i)
            {
                VisualObject* object = objectContainer.getData(i);
                if (!object->needsDisplayed_) continue;

                object->setVertexBuff(gD3dDeviceContext);
                object->setIndexBuff(gD3dDeviceContext);
                object->setTexture(gD3dDeviceContext, textureContainer, 0);

                object->updateAffineMat(gD3dDeviceContext, affineMatBuff);
                object->updateCBScene(gD3dDeviceContext, constantBuffer);

                object->drawIndex(gD3dDeviceContext);
            }

            // Full Screen Quad
            fsqTriangleObj->setVertexBuff(gD3dDeviceContext);
            fsqTriangleObj->setIndexBuff(gD3dDeviceContext);

            // Bloom
            gD3dDeviceContext->OMSetRenderTargets(1, bloomRT.view.GetAddressOf(), nullptr);
            gD3dDeviceContext->VSSetShader(FullScreenQuadVS.Get(), nullptr, 0);
            gD3dDeviceContext->PSSetShader(bloomPS.Get(), nullptr, 0);
            gD3dDeviceContext->PSSetShaderResources(0, 1, sceneRT.srv.GetAddressOf());
            fsqTriangleObj->drawIndex(gD3dDeviceContext);

            // Horiz Blur
            gD3dDeviceContext->OMSetRenderTargets(1, horizBlurRT.view.GetAddressOf(), nullptr);
            gD3dDeviceContext->VSSetShader(FullScreenQuadVS.Get(), nullptr, 0);
            gD3dDeviceContext->PSSetShader(horizBlurPS.Get(), nullptr, 0);
            gD3dDeviceContext->PSSetShaderResources(0, 1, bloomRT.srv.GetAddressOf());
            fsqTriangleObj->drawIndex(gD3dDeviceContext);

            // Vert Blur
            gD3dDeviceContext->OMSetRenderTargets(1, vertBlurRT.view.GetAddressOf(), nullptr);
            gD3dDeviceContext->VSSetShader(FullScreenQuadVS.Get(), nullptr, 0);
            gD3dDeviceContext->PSSetShader(vertBlurPS.Get(), nullptr, 0);
            gD3dDeviceContext->PSSetShaderResources(0, 1, horizBlurRT.srv.GetAddressOf());
            fsqTriangleObj->drawIndex(gD3dDeviceContext);

            // Combine
            gD3dDeviceContext->OMSetRenderTargets(1, &gMainRenderTargetView, nullptr);
            gD3dDeviceContext->VSSetShader(FullScreenQuadVS.Get(), nullptr, 0);
            gD3dDeviceContext->PSSetShader(combinePS.Get(), nullptr, 0);
            gD3dDeviceContext->PSSetShaderResources(0, 1, sceneRT.srv.GetAddressOf());
            gD3dDeviceContext->PSSetShaderResources(1, 1, vertBlurRT.srv.GetAddressOf());
            fsqTriangleObj->drawIndex(gD3dDeviceContext);
        }
        else
        {
            // Scene
            gD3dDeviceContext->OMSetRenderTargets(1, &gMainRenderTargetView, depthStencilView.Get());
            gD3dDeviceContext->ClearRenderTargetView(gMainRenderTargetView, clearColorWithAlpha);
            gD3dDeviceContext->VSSetShader(sceneVS.Get(), nullptr, 0);
            gD3dDeviceContext->PSSetShader(scenePS.Get(), nullptr, 0);

            // Draw objects and execute vertex shader
            for (u32 i = 0; i < objectContainer.getContainerSize(); ++i)
            {
                VisualObject* object = objectContainer.getData(i);
                if (!object->needsDisplayed_) continue;

                object->setVertexBuff(gD3dDeviceContext);
                object->setIndexBuff(gD3dDeviceContext);
                object->setTexture(gD3dDeviceContext, textureContainer, 0);

                object->updateAffineMat(gD3dDeviceContext, affineMatBuff);
                object->updateCBScene(gD3dDeviceContext, constantBuffer);

                object->drawIndex(gD3dDeviceContext);
            }
        }

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = gSwapChain->Present(1, 0);   // Present with vsync
        gSwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    /*************************************************************************************************************** */
    // Cleanup
    /*************************************************************************************************************** */
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

/******************************************************************************************************************** */
// Example's Helper functions
/******************************************************************************************************************** */
bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    u32 createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

    HRESULT res = D3D11CreateDeviceAndSwapChain
    (
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, 
        D3D11_SDK_VERSION, &sd, &gSwapChain, &D3DDevice(), &featureLevel, &gD3dDeviceContext
    );

    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain
        (
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, 
            D3D11_SDK_VERSION, &sd, &gSwapChain, &D3DDevice(), &featureLevel, &gD3dDeviceContext
        );

    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (gSwapChain) { gSwapChain->Release(); gSwapChain = nullptr; }
    if (gD3dDeviceContext) { gD3dDeviceContext->Release(); gD3dDeviceContext = nullptr; }
    if (D3DDevice()) D3DDevice() = nullptr;
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    gSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    D3DDevice()->CreateRenderTargetView(pBackBuffer, nullptr, &gMainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (gMainRenderTargetView) { gMainRenderTargetView->Release(); gMainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, u32 msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, u32 msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        gResizeWidth = (u32)LOWORD(lParam); // Queue resize
        gResizeHeight = (u32)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
