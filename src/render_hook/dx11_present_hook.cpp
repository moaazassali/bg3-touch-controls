#include <d3d11.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include "game_bindings.h"
#include "joystick/joystick.h"


typedef HRESULT(__stdcall* IDXGISwapChainPresent)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
IDXGISwapChainPresent original_DX11Present = nullptr;

// The index for Present in IDXGISwapChain is always 8
constexpr int PE_PRESENT_INDEX = 8;

// LLM Generated (not carefully checked but it works)
static void* GetDX11PresentAddress()
{
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = DefWindowProcW; // Use DefWindowProcW
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"BG3DummyWindow"; // L prefix works with W structures

    if (!RegisterClassExW(&wc)) // Use RegisterClassExW
    {
        return nullptr;
    }

    // 2. Use CreateWindowW
    HWND hWnd = CreateWindowW(
        wc.lpszClassName,
        L"BG3 Dummy",
        WS_OVERLAPPEDWINDOW,
        0, 0, 100, 100,
        NULL, NULL, wc.hInstance, NULL
    );

    if (!hWnd)
    {
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return nullptr;
    }

    // 3. Prepare D3D11 initialization parameters
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };

    DXGI_SWAP_CHAIN_DESC sd = { 0 };
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Standard format
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ID3D11Device* pDevice = nullptr;
    ID3D11DeviceContext* pContext = nullptr;
    IDXGISwapChain* pSwapChain = nullptr;

    // 4. Create the Device and SwapChain
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        0,
        featureLevels,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &pSwapChain,
        &pDevice,
        &featureLevel,
        &pContext
    );

    void* pPresentAddress = nullptr;

    if (SUCCEEDED(hr) && pSwapChain)
    {
        // 5. Get the VTable (Virtual Function Table)
        // The SwapChain pointer points to the VTable pointer
        void** vTable = *reinterpret_cast<void***>(pSwapChain);

        // 6. Get the address of Present (Index 8)
        pPresentAddress = vTable[PE_PRESENT_INDEX];

        // Cleanup D3D objects immediately
        pSwapChain->Release();
        pDevice->Release();
        pContext->Release();
    }

    // 7. Cleanup Window
    DestroyWindow(hWnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return pPresentAddress;
}


bool init = false;
ID3D11Device* pDevice = nullptr;
ID3D11DeviceContext* pContext = nullptr;
ID3D11RenderTargetView* mainRenderTargetView = nullptr;
HWND window = nullptr;

// Mostly LLM generated
static void initializeImGui(IDXGISwapChain* pSwapChain)
{
    // Get the device and context from the SwapChain
    if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
    {
        pDevice->GetImmediateContext(&pContext);

        // Get the Window Handle (BG3 Window)
        DXGI_SWAP_CHAIN_DESC sd;
        pSwapChain->GetDesc(&sd);
        window = sd.OutputWindow;

        // Create a Render Target View (Required for ImGui to know where to draw)
        ID3D11Texture2D* pBackBuffer = nullptr;
        if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer)) && pBackBuffer)
        {
            if (SUCCEEDED(pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView)))
            {
                // Initialize ImGui
                ImGui::CreateContext();
                ImGuiIO& io = ImGui::GetIO();

                ImGui_ImplWin32_Init(window);
                ImGui_ImplDX11_Init(pDevice, pContext);

                init = true;
            }
            pBackBuffer->Release();
        }
    }
}



static HRESULT __stdcall detour_DX11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (!init)
    {
        initializeImGui(pSwapChain);
        return original_DX11Present(pSwapChain, SyncInterval, Flags);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // We use GetBackgroundDrawList to draw directly on the screen without a window background
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    // =============================
    // Our custom renderers go here
    renderJoystick(drawList);
    // =============================

    ImGui::Render();

    // Bind the render target and draw
    pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return original_DX11Present(pSwapChain, SyncInterval, Flags);
}


bool installDX11PresentHook()
{
	auto pTarget = GetDX11PresentAddress();
	return installHook(pTarget, &detour_DX11Present, (void**)&original_DX11Present, "DX11 Present");
}