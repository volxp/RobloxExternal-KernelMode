#pragma once

#include <Windows.h>
#include <string>
#include <d3d11.h>
#include <memory>
#include "overlay.h"
#include "../Dependencies/imgui/imgui.h"
#include "../Dependencies/imgui/imgui_impl_win32.h"
#include "../Dependencies/imgui/imgui_impl_dx11.h"

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
// Implementation

Overlay::Overlay() {
    ZeroMemory(&windowClass, sizeof(WNDCLASSEX));
}

Overlay::~Overlay() {
    Shutdown();
}

bool Overlay::Initialize() {
    if (!CreateOverlayWindow())
        return false;

    if (!InitializeDirectX())
        return false;

    if (!InitializeImGui())
        return false;

    running = true;
    return true;
}
//'getenv': This function or variable may be unsafe. Consider using _dupenv_s instead. To disable deprecation, use . See online help for details.
bool Overlay::CreateOverlayWindow() {
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = GetModuleHandle(NULL);
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
    windowClass.lpszClassName = "RobloxOverlayClass";

    if (!RegisterClassEx(&windowClass))
        return false;

    // Get the screen width and height
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Create window with WS_EX_LAYERED but not WS_EX_TRANSPARENT initially
    // This allows interaction when the menu is shown
    overlayWindow = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        windowClass.lpszClassName,
        "Roblox",
        WS_POPUP,
        0, 0, // Position the window at the top-left corner of the screen
        screenWidth, screenHeight, // Fullscreen size
        NULL,
        NULL,
        windowClass.hInstance,
        this
    );

    if (!overlayWindow)
        return false;

    SetLayeredWindowAttributes(overlayWindow, RGB(0, 0, 0), 0, LWA_COLORKEY);

    // Set window to top and make it visible
    ShowWindow(overlayWindow, SW_SHOW);
    SetWindowPos(overlayWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    return true;
}

bool Overlay::InitializeDirectX() {
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = 0;
    swapChainDesc.BufferDesc.Height = 0;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = overlayWindow;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevelArray, 2,
        D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device, &featureLevel, &deviceContext
    );

    if (FAILED(result))
        return false;

    // Create render target
    ID3D11Texture2D* backBuffer;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
    device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
    backBuffer->Release();

    return true;
}

bool Overlay::InitializeImGui() {
    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f); // Better font

    // Setup custom style
    SetupImGuiStyle();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(overlayWindow);
    ImGui_ImplDX11_Init(device, deviceContext);

    return true;
}

void Overlay::SetupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;

    // Farben
    ImVec4 red = ImVec4(0.85f, 0.1f, 0.1f, 1.0f); // Kräftigeres Rot
    ImVec4 bg = ImVec4(0.08f, 0.08f, 0.08f, 0.95f); // Fast schwarzer Hintergrund
    ImVec4 text = ImVec4(0.95f, 0.95f, 0.95f, 1.0f); // Fast weißer Text
    ImVec4 frameBg = ImVec4(0.15f, 0.15f, 0.15f, 1.0f); // Slider/Inputs

    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg] = bg;
    colors[ImGuiCol_Text] = text;

    colors[ImGuiCol_Button] = red;
    colors[ImGuiCol_ButtonHovered] = red;   // Kein Hover-Farbwechsel
    colors[ImGuiCol_ButtonActive] = red;   // Kein Klick-Farbwechsel

    colors[ImGuiCol_FrameBg] = frameBg;
    colors[ImGuiCol_FrameBgHovered] = frameBg;
    colors[ImGuiCol_FrameBgActive] = frameBg;

    colors[ImGuiCol_SliderGrab] = red;
    colors[ImGuiCol_SliderGrabActive] = red;

    colors[ImGuiCol_Separator] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    colors[ImGuiCol_ResizeGrip] = red;
    colors[ImGuiCol_ResizeGripHovered] = red;
    colors[ImGuiCol_ResizeGripActive] = red;

    colors[ImGuiCol_TitleBg] = bg;
    colors[ImGuiCol_TitleBgCollapsed] = bg;
    colors[ImGuiCol_TitleBgActive] = bg;
}


void Overlay::Run() {
    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));

    while (running && msg.message != WM_QUIT) {
        // Process input - check for RIGHT SHIFT key press to toggle menu
        if (GetAsyncKeyState(VK_RSHIFT) & 1) {
            showMenu = !showMenu;

            // Toggle transparent window style based on menu state
            LONG exStyle = GetWindowLong(overlayWindow, GWL_EXSTYLE);
            if (showMenu) {
                // Menu is visible - remove transparent flag to allow interaction
                exStyle &= ~WS_EX_TRANSPARENT;
            }
            else {
                // Menu is hidden - add transparent flag to allow click-through
                exStyle |= WS_EX_TRANSPARENT;
            }
            SetWindowLong(overlayWindow, GWL_EXSTYLE, exStyle);
        }

        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        Render();
    }
}

void Overlay::Render() {
    // Clear the render target
    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    deviceContext->ClearRenderTargetView(renderTargetView, clearColor);
    deviceContext->OMSetRenderTargets(1, &renderTargetView, nullptr);

    // Start ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Render ImGui elements
    RenderImGui();

    // Render ImGui
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Present the frame
    swapChain->Present(0, 0);
}

void Overlay::RenderImGui() {
    if (!showMenu)
        return;

    // Set window flags to make it draggable and with a custom title bar
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings;

    // Create a main window that's draggable
    ImGui::Begin("Cloudy coolui", &showMenu, window_flags);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.11f, 0.51f, 0.81f, 1.0f));
    ImGui::Text("Player Modifications");
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Add Health slider with label and value display


    healthValue = driver::read_mem<float>(driver_handle, Globals::humanoid + Offsets::health);

	


    ImGui::Text("Health");
    if (ImGui::SliderFloat("##Health", &healthValue, 0.0f, 500.0f, "%.1f")) {
        actions::Health(healthValue);
    }
    ImGui::SameLine();

    // Visual indicator of health
    float healthPercent = healthValue / 100.0f;
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
        ImVec4(1.0f - healthPercent, healthPercent, 0.0f, 0.7f)); // Red to green color
    ImGui::ProgressBar(healthPercent, ImVec2(100, 15), "");
    ImGui::PopStyleColor();

    ImGui::Spacing();

    ImGui::Text("Walk Speed");
	walkSpeedValue = driver::read_mem<float>(driver_handle, Globals::humanoid + Offsets::walkspeed);
    if (ImGui::SliderFloat("##WalkSpeed", &walkSpeedValue, 1.0f, 150.0f, "%.1f")) {
        actions::Walkspeed(walkSpeedValue);
    }
    ImGui::SameLine();
    ImGui::Text("%0.1fx", walkSpeedValue / 16.0f);

    ImGui::Spacing();
	ImGui::Text("Jump Power");
	jumpPowerValue = driver::read_mem<float>(driver_handle, Globals::humanoid + Offsets::jumppower);
	if (ImGui::SliderFloat("##JumpPower", &jumpPowerValue, 0.0f, 200.0f, "%.1f")) {
		actions::JumpPower(jumpPowerValue);
	}
	ImGui::SameLine();
	ImGui::Text("%0.1fx", jumpPowerValue / 50.0f);
	ImGui::Spacing();

    ImGui::Text("Gravity");
    Gravity = driver::read_mem<float>(driver_handle, Globals::workspace + Offsets::gravity);
	if (ImGui::SliderFloat("##Gravity", &Gravity, 0.0f, 200.0f, "%.1f")) {
		actions::Gravity(Gravity);
	}


	ImGui::SameLine();
	ImGui::Text("%0.1fx", Gravity / 10.0f);
	ImGui::Spacing();

    ImGui::Separator();
    ImGui::Spacing();

    // Add some information and controls at the bottom
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Toggle with RIGHT SHIFT");

    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
    if (ImGui::Button("Close Overlay", ImVec2(90, 25))) {
        running = false;
    }

    ImGui::End();
}

void Overlay::Shutdown() {
    if (!running)
        return;

    running = false;

    // Cleanup ImGui
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // Cleanup DirectX
    if (renderTargetView) { renderTargetView->Release(); renderTargetView = nullptr; }
    if (swapChain) { swapChain->Release(); swapChain = nullptr; }
    if (deviceContext) { deviceContext->Release(); deviceContext = nullptr; }
    if (device) { device->Release(); device = nullptr; }

    // Destroy window
    if (overlayWindow) {
        DestroyWindow(overlayWindow);
        overlayWindow = nullptr;
    }

    // Unregister window class
    UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

LRESULT CALLBACK Overlay::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Forward mouse & keyboard messages to ImGui
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    // Get the overlay instance
    Overlay* overlay = (Overlay*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (msg) {
    case WM_SIZE:
        if (overlay && overlay->device != nullptr && wParam != SIZE_MINIMIZED) {
            // Release render target
            if (overlay->renderTargetView) {
                overlay->renderTargetView->Release();
                overlay->renderTargetView = nullptr;
            }

            // Resize swap chain buffers
            overlay->swapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);

            // Create new render target
            ID3D11Texture2D* backBuffer;
            overlay->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
            overlay->device->CreateRenderTargetView(backBuffer, nullptr, &overlay->renderTargetView);
            backBuffer->Release();
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_CREATE:
        // Store the overlay pointer for future use
        if (CREATESTRUCT* cs = (CREATESTRUCT*)lParam) {
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        }
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}