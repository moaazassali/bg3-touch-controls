#pragma once
#include <imgui.h>


struct JoystickState
{
	bool isActive = false;
	// coordinates are in physical pixels relative to game window's top-left corner
	float posX = 0.0f;
	float posY = 0.0f;
	// strength values are in range [-1.0, 1.0]
	float strengthX = 0.0f;
	float strengthY = 0.0f;
};
extern JoystickState g_joystickState;

void activateJoystick(float startX, float startY);
void releaseJoystick();
void moveJoystick(float translationX, float translationY);

void renderJoystick(ImDrawList* drawList);