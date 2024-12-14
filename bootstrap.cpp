#include "bootstrap.h"
#include <windows.h>
#include "overlay/imgui/imgui.h"
#include "overlay/imgui/imgui_impl_dx11.h"
#include "overlay/imgui/imgui_impl_win32.h"
#include <d3d11.h>
#include <tchar.h>
#include <stdexcept>
#include "menu.h"
#include <iostream>

// Static variables for DirectX
ID3D11Device* Bootstrap::g_pd3dDevice = nullptr;
ID3D11DeviceContext* Bootstrap::g_pd3dDeviceContext = nullptr;
IDXGISwapChain* Bootstrap::g_pSwapChain = nullptr;
ID3D11RenderTargetView* Bootstrap::g_mainRenderTargetView = nullptr;
HWND Bootstrap::g_hwdn = nullptr;

bool exit_ready = false;
bool clear_screen = false;

bool Bootstrap::Initialize() {
    auto menu_name = "L0ckedIn Lockd0wn - sean3z";

    // Create a new window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, Bootstrap::WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T(menu_name), NULL };
    RegisterClassEx(&wc);

    // Window properties and creation
    g_hwdn = CreateWindowEx(
        0, wc.lpszClassName, _T(menu_name),
        WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, wc.hInstance, NULL
    );

    if (g_hwdn == NULL) {
        throw std::runtime_error("Failed creating window");
    }

    CreateDeviceD3D();

    ShowWindow(g_hwdn, SW_SHOWNORMAL);
    UpdateWindow(g_hwdn);

    InitializeImGui();

    return true;
}

void Bootstrap::InitializeImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    ImGui::GetIO().WantSaveIniSettings = false;
    ImGui::GetIO().IniFilename = "";

    ImGui::StyleColorsDark();

    // Customize Style
    ImGuiStyle& style = ImGui::GetStyle();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 13.f);

    if (font == nullptr) {
        throw std::runtime_error("Failed to load font!");
        return;
    }

    ImGui_ImplWin32_Init(g_hwdn);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    CreateRenderTarget();
}

void Bootstrap::CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    
    if (pBackBuffer) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    }
    
    pBackBuffer->Release();
}

void Bootstrap::Cleanup() {
    // Clean up ImGui resources
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // Clean up DirectX resources
    CleanupRenderTarget();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pd3dDeviceContext) g_pd3dDeviceContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
}

void Bootstrap::CleanupRenderTarget() {
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

LRESULT CALLBACK Bootstrap::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
        case WM_NCHITTEST:
        {
            // Call default processing first
            LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);
            
            // If the hit test result is within the client area, do not override it
            if (hit == HTCLIENT)
            {
                // Optional: Use additional logic to specify specific areas to make draggable
                RECT rect;
                GetClientRect(hWnd, &rect);
                
                POINT pt = { LOWORD(lParam), HIWORD(lParam) };
                ScreenToClient(hWnd, &pt);

                // Define your draggable region (example: top 30 pixels as "title bar")
                if (pt.y < 30)  // Assuming top 30 pixels are reserved for dragging
                    return HTCAPTION;
            }

            return hit;
        }
        
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

MSG Bootstrap::begin_frame() {
    MSG msg{ 0 };

    if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    return msg;   
}

void Bootstrap::end_frame() {
    // Render your ImGui frame
    ImGui::Render();
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);

    float clearColor[4] = { 0.45f, 0.55f, 0.60f, 1.00f };  // RGBA color
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clearColor);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Present frame (if using DX11)
    g_pSwapChain->Present(1, 0); // Swap buffers
}

bool Bootstrap::RenderLoop() {
    if (exit_ready)
        return false;

    if (g_hwdn && !clear_screen) {
        auto msg = begin_frame();

        Menu::RenderMenu();
        
        end_frame();

        return msg.message != WM_QUIT;
    }
    else if (clear_screen) {
        auto msg = begin_frame();
        end_frame();

        //std::this_thread::sleep_for(std::chrono::seconds(1));

        clear_screen = false;

        return msg.message != WM_QUIT;
    }
    return false;
}

void Bootstrap::exit() {
    begin_frame();
    end_frame();
    exit_ready = true;
}

bool Bootstrap::CreateDeviceD3D()
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
    sd.OutputWindow = g_hwdn;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}