#ifndef BOOTSTRAP_H
#define BOOTSTRAP_H

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"dwmapi.lib")
#pragma comment(lib,"d3dcompiler")

#include <d3d11.h>
#include "overlay/imgui/imgui.h"
#include <windows.h>

class Bootstrap {
public:

    static ID3D11Device* g_pd3dDevice;
    static ID3D11DeviceContext* g_pd3dDeviceContext;
    static IDXGISwapChain* g_pSwapChain;
    static ID3D11RenderTargetView* g_mainRenderTargetView;
    static HWND g_hwdn;

    // Initializes DirectX
    static bool Initialize();

    // Initializes ImGui
    static void InitializeImGui();

    // Creates the render target for DirectX
    static void CreateRenderTarget();

    // Cleans up DirectX and ImGui resources
    static void Cleanup();

    // Cleans up the render target
    static void CleanupRenderTarget();

    // Runs the render loop: processes events and renders the UI
    static bool RenderLoop();

    static bool CreateDeviceD3D();

    static MSG begin_frame();
    static void end_frame();
    static void exit();

    // The Windows procedure that handles window events
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

};

#endif // BOOTSTRAP_H
