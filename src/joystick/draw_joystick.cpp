
#include <d3d11.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>


typedef HRESULT(__stdcall* IDXGISwapChainPresent)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
extern IDXGISwapChainPresent original_DX11Present;

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


static float calculateJoystickRadius(float windowWidth, float windowHeight)
{
	constexpr float phoneRadiusScaleFactor = 0.12f;
	return min(windowWidth, windowHeight) * phoneRadiusScaleFactor;
}


HRESULT __stdcall detour_DX11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
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

	constexpr int circleSegments = 64;
	constexpr float circleThickness = 5.0f;
	constexpr float innerCircleRadiusFactor = 0.65f;
	constexpr auto circleOutlineColor = IM_COL32(104, 74, 56, 255);
	constexpr auto circleFillColor = IM_COL32(104, 74, 56, 128);

	auto& io = ImGui::GetIO();
	float offset = min(io.DisplaySize.x, io.DisplaySize.y) * 0.45f;
	auto center = ImVec2(offset, offset);
	// Joystick radius in logical coordinates
	float radius = calculateJoystickRadius(io.DisplaySize.x, io.DisplaySize.y);
	drawList->AddCircle(center, radius, circleOutlineColor, circleSegments, circleThickness);
	drawList->AddCircleFilled(center, radius * innerCircleRadiusFactor, circleFillColor, circleSegments);

	ImGui::Render();

	// Bind the render target and draw
	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return original_DX11Present(pSwapChain, SyncInterval, Flags);
}