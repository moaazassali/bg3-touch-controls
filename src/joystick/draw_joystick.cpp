
#include <d3d11.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <algorithm>
#include <iostream>

#include "joystick/joystick.h"


typedef HRESULT(__stdcall* IDXGISwapChainPresent)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
extern IDXGISwapChainPresent original_DX11Present;

bool init = false;
ID3D11Device* pDevice = nullptr;
ID3D11DeviceContext* pContext = nullptr;
ID3D11RenderTargetView* mainRenderTargetView = nullptr;
HWND window = nullptr;

JoystickState g_joystickState = {};


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


static float calculateJoystickRadius()
{
	constexpr float radiusInCm = 1.0f;
	constexpr float inchesInCm = 0.3937008f;
	constexpr int DPI = 353; // Should match TouchHandler::DPI and be configurable
	constexpr float radiusInPx = radiusInCm * inchesInCm * DPI;
	return radiusInPx;
}


void activateJoystick(float startX, float startY)
{
	g_joystickState.isActive = true;
	g_joystickState.posX = startX;
	g_joystickState.posY = startY;
	g_joystickState.strengthX = 0;
	g_joystickState.strengthY = 0;
}


void releaseJoystick()
{
	g_joystickState.isActive = false;
	g_joystickState.posX = 0.0f;
	g_joystickState.posY = 0.0f;
	g_joystickState.strengthX = 0;
	g_joystickState.strengthY = 0;
}


// x and y are the physical pixel translation deltas of user swipe input
// We move the joystick in a touch following manner.
// Specifically, if the joystick is moved beyond its maximum radius, then the entire joystick
// is shifted in that direction to keep the knob within the radius.
// This way, the user can still switch directions without having to go all the way back to center first.
void moveJoystick(float translationX, float translationY)
{
	if (!g_joystickState.isActive) {
		return;
	}

	// Invert translationY because screen Y+ is down but joystick Y+ is up
	translationY = -translationY;

	float radiusPx = calculateJoystickRadius();

	auto oldStrengthXInPx = g_joystickState.strengthX * radiusPx;
	auto oldStrengthYInPx = g_joystickState.strengthY * radiusPx;

	// Inner circle cannot exceed radius, so clamp it
	auto newStrengthXInPx = std::clamp(oldStrengthXInPx + translationX, -radiusPx, radiusPx);
	auto newStrengthYInPx = std::clamp(oldStrengthYInPx + translationY, -radiusPx, radiusPx);

	// Convert back to normalized strength values [-1.0, 1.0]
	auto newStrengthX = newStrengthXInPx / radiusPx;
	auto newStrengthY = newStrengthYInPx / radiusPx;

	// Calculate any extra translation beyond the radius limit that was done
	// to apply that to the joystick position
	auto extraXInPx = translationX - (newStrengthXInPx - oldStrengthXInPx);
	auto extraYInPx = translationY - (newStrengthYInPx - oldStrengthYInPx);

	g_joystickState.posX += extraXInPx;
	// To move joystick up on screen, we have to decrease its Y coordinate and vice versa
	g_joystickState.posY -= extraYInPx;
	g_joystickState.strengthX = newStrengthX;
	g_joystickState.strengthY = newStrengthY;
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

	// Only the draw logic happens if the joystick is active
	// You could ask why not also skip everything else and call original_DX11Present directly?
	// The reason is that some fields like io.DisplaySize will be uninitialized if we skip the ImGui NewFrame calls
	// For instance, io.DisplaySize.x will be -1, so we can't conditionally activate the joystick based on that in
	// activateJoystick().
	if (g_joystickState.isActive) {

		// We use GetBackgroundDrawList to draw directly on the screen without a window background
		ImDrawList* drawList = ImGui::GetBackgroundDrawList();

		constexpr int circleSegments = 64;
		constexpr float circleThickness = 5.0f;
		constexpr float innerCircleRadiusFactor = 0.90f;
		constexpr auto circleOutlineColor = IM_COL32(96, 80, 70, 255);
		constexpr auto circleFillColor = IM_COL32(96, 80, 70, 128);

		auto& io = ImGui::GetIO();
		auto center = ImVec2(g_joystickState.posX / io.DisplayFramebufferScale.x, g_joystickState.posY / io.DisplayFramebufferScale.y);
		float radiusPx = calculateJoystickRadius();
		// Convert radius to logical units
		auto radius = radiusPx / io.DisplayFramebufferScale.x;
		drawList->AddCircle(center, radius, circleOutlineColor, circleSegments, circleThickness);

		auto innerCircleCenter = ImVec2(
			(center.x + g_joystickState.strengthX * radius) / io.DisplayFramebufferScale.x,
			// Minus instead because we invert strengthY in moveJoystick
			(center.y - g_joystickState.strengthY * radius) / io.DisplayFramebufferScale.y
		);
		drawList->AddCircleFilled(innerCircleCenter, radius * innerCircleRadiusFactor, circleFillColor, circleSegments);
	}

	ImGui::Render();

	// Bind the render target and draw
	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return original_DX11Present(pSwapChain, SyncInterval, Flags);
}