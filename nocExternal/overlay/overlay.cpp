#include "overlay.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx11.h"
#include <dwmapi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Overlay::Overlay() {}

Overlay::~Overlay()
{
    Cleanup();
}

bool Overlay::Initialize()
{
    CreateOverlayWindow();
    InitializeDirectX();
    InitializeImGui();
    return true;
}

void Overlay::CreateOverlayWindow()
{
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); 
    wc.lpszClassName = "nocExternal";
    RegisterClassEx(&wc);
    
    window_handle = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        "nocExternal", "noc-external",
        WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    
    // SAFE TRANSPARENCY METHOD
    // Making the black color (0,0,0) transparent. This is the most compatible way to prevent black screens.
    SetLayeredWindowAttributes(window_handle, RGB(0, 0, 0), 255, LWA_COLORKEY);
    
    // Extend for modern DWM compatibility
    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(window_handle, &margins);
    
    ShowWindow(window_handle, SW_SHOW);
    UpdateWindow(window_handle);
}

void Overlay::InitializeDirectX()
{
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1; // Legacy model for maximum compatibility (prevents black screen)
    scd.BufferDesc.Width = GetSystemMetrics(SM_CXSCREEN);
    scd.BufferDesc.Height = GetSystemMetrics(SM_CYSCREEN);
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 0; // Auto
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = window_handle;
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // Discard is safer for transparency-keyed windows
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    
    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &scd, &swapchain, &device, &featureLevel, &context);

    ID3D11Texture2D* back_buffer;
    swapchain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    device->CreateRenderTargetView(back_buffer, NULL, &render_target_view);
    back_buffer->Release();
}

void Overlay::InitializeImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    ImGui::StyleColorsDark();
    
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.AntiAliasedLines = true;
    style.AntiAliasedFill = true;

    ImGui_ImplWin32_Init(window_handle);
    ImGui_ImplDX11_Init(device, context);
}

void Overlay::BeginFrame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    context->OMSetRenderTargets(1, &render_target_view, NULL);
    // Clear to perfect black. Because of LWA_COLORKEY, this black will become transparent.
    static const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    context->ClearRenderTargetView(render_target_view, clear_color);
}

void Overlay::EndFrame()
{
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    
    // Present with VSync off (0) for minimum input latency.
    swapchain->Present(0, 0); 
}

void Overlay::DrawText(Vector2 position, const char* text, unsigned int color)
{
    ImGui::GetForegroundDrawList()->AddText(ImVec2(position.x, position.y), color, text);
}

void Overlay::SetClickThrough(bool click_through)
{
    if (!window_handle) return;
    
    LONG_PTR ex_style = GetWindowLongPtr(window_handle, GWL_EXSTYLE);
    if (click_through)
    {
        ex_style |= WS_EX_TRANSPARENT;
    }
    else
    {
        ex_style &= ~WS_EX_TRANSPARENT;
    }
    SetWindowLongPtr(window_handle, GWL_EXSTYLE, ex_style);
}

bool Overlay::ShouldExit()
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT) return true;
    }
    return false;
}

bool Overlay::IsRunning()
{
    return !ShouldExit();
}

LRESULT CALLBACK Overlay::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
        return true;
    
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Overlay::Cleanup()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    
    if (render_target_view) render_target_view->Release();
    if (swapchain) swapchain->Release();
    if (context) context->Release();
    if (device) device->Release();
    
    if (window_handle) DestroyWindow(window_handle);
}