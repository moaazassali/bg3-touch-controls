#include <d3d11.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <algorithm>
#include <iostream>

#include "joystick/joystick.h"
#include "touch_handler.h"


JoystickState g_joystickState = {};


static float calculateJoystickRadius()
{
	constexpr float radiusInCm = 1.0f;
	const float radiusInPx = TouchHandler::cmToPixels(radiusInCm);
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


void renderJoystick(ImDrawList* drawList)
{
	// Only the draw logic happens if the joystick is active
	// You could ask why not also skip everything else and call original_DX11Present directly?
	// The reason is that some fields like io.DisplaySize will be uninitialized if we skip the ImGui NewFrame calls
	// For instance, io.DisplaySize.x will be -1, so we can't conditionally activate the joystick based on that in
	// activateJoystick().
	if (g_joystickState.isActive) {
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
}