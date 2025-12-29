#include <imgui.h>

#include "ability_usage.h"
#include "touch_handler.h"


// Mostly LLM generated
struct ConfirmButtonState {
	bool isPressed = false;
	float centerX = 0.0f;
	float centerY = 0.0f;
	float radius = 0.0f;
};
static ConfirmButtonState g_buttonState{};


static float calculateButtonRadius()
{
	constexpr float radiusInCm = 0.75f;
	const float radiusInPx = TouchHandler::cmToPixels(radiusInCm);
	return radiusInPx;
}


void pressInConfirmButton()
{
	g_buttonState.isPressed = true;
}


void pressOutConfirmButton()
{
	g_buttonState.isPressed = false;
}


bool isPointInsideConfirmButton(float x, float y)
{
	if (!isAnAbilityPrimed()) {
		return false;
	}

	const float dx = x - g_buttonState.centerX;
	const float dy = y - g_buttonState.centerY;
	const float distanceSquared = dx * dx + dy * dy;
	const float radiusSquared = g_buttonState.radius * g_buttonState.radius;

	return distanceSquared <= radiusSquared;
}


ConfirmButtonState getConfirmButtonState()
{
	return g_buttonState;
}


static void drawCheckmark(ImDrawList* drawList, ImVec2 center, float size, ImU32 color, float thickness)
{
	// Draw a checkmark: short line going down-left, then longer line going up-right
	// Short tail: 35% of size, Long tail: 85% of size
	// Angles: short tail at 60° down, long tail at 35° up for tighter angle

	const float shortLineLength = size * 0.35f;
	const float longLineLength = size * 0.85f;

	// Starting point (left side of checkmark)
	const ImVec2 startPoint = ImVec2(
		center.x - size * 0.35f,
		center.y
	);

	// Middle point (bottom of checkmark) - 60° angle (steeper descent)
	const ImVec2 middlePoint = ImVec2(
		startPoint.x + shortLineLength * 0.5f,    // cos(60°)
		startPoint.y + shortLineLength * 0.866f   // sin(60°)
	);

	// End point (top-right of checkmark) - 35° angle (less steep ascent)
	const ImVec2 endPoint = ImVec2(
		middlePoint.x + longLineLength * 0.819f,  // cos(35°)
		middlePoint.y - longLineLength * 0.574f   // sin(-35°)
	);

	// Draw the two lines
	drawList->AddLine(startPoint, middlePoint, color, thickness);
	drawList->AddLine(middlePoint, endPoint, color, thickness);
}


void renderConfirmAbilityButton(ImDrawList* drawList)
{
	// Only render the button when an ability is primed
	if (!isAnAbilityPrimed()) {
		g_buttonState.isPressed = false;
		return;
	}

	constexpr int circleSegments = 64;
	constexpr float circleThickness = 4.0f;

	// Color scheme changes based on press state
	const ImU32 circleOutlineColor = g_buttonState.isPressed
		? IM_COL32(255, 220, 100, 255)  // Brighter golden when pressed
		: IM_COL32(200, 180, 50, 255);   // Normal golden

	const ImU32 circleFillColor = g_buttonState.isPressed
		? IM_COL32(180, 140, 40, 240)    // Darker, more opaque when pressed
		: IM_COL32(150, 120, 30, 200);   // Normal fill

	const ImU32 checkmarkColor = g_buttonState.isPressed
		? IM_COL32(255, 255, 255, 255)   // White when pressed
		: IM_COL32(240, 220, 120, 255);  // Light golden normally

	auto& io = ImGui::GetIO();

	// Calculate button position: 3cm from right edge, centered vertically
	constexpr float distanceFromRightEdgeInCm = 3.0f;
	const float distanceFromRightEdgeInPx = TouchHandler::cmToPixels(distanceFromRightEdgeInCm);
	const float radiusPx = calculateButtonRadius();

	// Position in physical pixels
	const float buttonPosX = (io.DisplaySize.x * io.DisplayFramebufferScale.x) - distanceFromRightEdgeInPx;
	const float buttonPosY = (io.DisplaySize.y * io.DisplayFramebufferScale.y) / 2.0f;

	// Store physical pixel coordinates for hit testing
	g_buttonState.centerX = buttonPosX;
	g_buttonState.centerY = buttonPosY;
	g_buttonState.radius = radiusPx;

	// Convert to logical units for ImGui rendering
	const auto center = ImVec2(
		buttonPosX / io.DisplayFramebufferScale.x,
		buttonPosY / io.DisplayFramebufferScale.y
	);
	const auto radius = radiusPx / io.DisplayFramebufferScale.x;

	// Draw filled circle
	drawList->AddCircleFilled(center, radius, circleFillColor, circleSegments);

	// Draw outline
	drawList->AddCircle(center, radius, circleOutlineColor, circleSegments, circleThickness);

	// Draw checkmark icon
	const float checkmarkSize = radius * 0.9f;
	const float checkmarkThickness = 5.0f;
	drawCheckmark(drawList, center, checkmarkSize, checkmarkColor, checkmarkThickness);
}