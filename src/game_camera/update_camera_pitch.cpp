#include <iostream>

#include "game_bindings.h"
#include "utils.h"


// Adapted from BG3_NativeCameraTweaks
using updateCameraPitch_t = void(*)(uint64_t a1, uint64_t a2, CameraObject* a3, DeltaTimeObject* a4);
updateCameraPitch_t original_updateCameraPitch = nullptr;
// Adapted from BG3_NativeCameraTweaks
using getCameraDefinition_t = CameraDefinition * (*)(CameraObject* a1);
getCameraDefinition_t original_getCameraDefinition = nullptr;


// This gets called every frame in original_updateCamera
static void detour_updateCameraPitch(uint64_t a1, uint64_t a2, CameraObject* a3, DeltaTimeObject* a4)
{
	if (a3) {
		const auto camDef = original_getCameraDefinition(a3);
		const auto originalA = camDef->pitchAdjustSpeedA_48;
		const auto originalB = camDef->pitchAdjustSpeedB_F0;
		const auto originalC = camDef->pitchAdjustSpeedC_F4;

		// Set to very high values to make pitch change instant
		// Otherwise, the game will try to smoothly interpolate to our desired pitch over a few seconds.
		// We want the changes within this frame to be near-instant.
		camDef->pitchAdjustSpeedA_48 = 100000.0f;
		camDef->pitchAdjustSpeedB_F0 = 100000.0f;
		camDef->pitchAdjustSpeedC_F4 = 100000.0f;

		original_updateCameraPitch(a1, a2, a3, a4);

		camDef->pitchAdjustSpeedA_48 = originalA;
		camDef->pitchAdjustSpeedB_F0 = originalB;
		camDef->pitchAdjustSpeedC_F4 = originalC;
	}
	else original_updateCameraPitch(a1, a2, a3, a4);
}


bool getCameraDefinitionPointer(uintptr_t start, uintptr_t end)
{
	constexpr const char* getCameraDefinitionPattern = "48 8B 05 ?? ?? ?? ?? 80 B8 32 13 00 00 00 74 07";
	auto matchAddr = findPattern(getCameraDefinitionPattern, start, end);
	if (!matchAddr) {
		printPatternNotFoundError(getCameraDefinitionPattern, "getCameraDefinition");
		return false;
	}
	original_getCameraDefinition = reinterpret_cast<getCameraDefinition_t>(matchAddr);
	return true;
}


bool installUpdateCameraPitchHook(uintptr_t start, uintptr_t end)
{
	constexpr const char* updateCameraPitchPattern = "E8 ?? ?? ?? ?? 48 8B 46 70 4C 8D 45 90 0F 28 46 30";
	auto matchAddr = findPattern(updateCameraPitchPattern, start, end);
	auto relativeOffset = *reinterpret_cast<int32_t*>(matchAddr + 1);
	auto targetAddr = matchAddr + 5 + relativeOffset;
	return installHook((void*)targetAddr, &detour_updateCameraPitch, (void**)&original_updateCameraPitch, "updateCameraPitch");
}