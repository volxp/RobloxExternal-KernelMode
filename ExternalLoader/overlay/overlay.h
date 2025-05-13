#pragma once
#include <Windows.h>
#include <d3d11.h>
#include "../TaskScheduler/Memory.hpp"
class Overlay {
public:
    Overlay();
    ~Overlay();
    bool Initialize();
    void Run();
    void Shutdown();
    inline bool IsRunning() const { return running; }

private:
    HWND overlayWindow;
    WNDCLASSEX windowClass;
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // DirectX objects
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* deviceContext = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11RenderTargetView* renderTargetView = nullptr;

    // State variables
    bool running = false;
    bool showMenu = true;

    // UI values
    float healthValue = 100.0f;  // Default health value
    float walkSpeedValue = 16.0f;  // Default walk speed
    float jumpPowerValue = 50.0f;
    // Initialization methods
    bool CreateOverlayWindow();
    bool InitializeDirectX();
    bool InitializeImGui();
    void SetupImGuiStyle();

    // Rendering methods
    void Render();
    void RenderImGui();
};