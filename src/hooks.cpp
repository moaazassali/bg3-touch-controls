#include <windows.h>
#include <iostream>
#include <Psapi.h>
#include <vector>
#include <MinHook.h>
#include <algorithm>

#include "RE/Camera.h"
#include "pending_camera_updates.h"
#include "Hooks/hooks_utils.h"
#include "Hooks/update_camera_hook.h"
#include "Hooks/calculate_pitch_hook.h"


using getCameraDefinition_t = RE::CameraDefinition* (*)(RE::CameraObject* a1);
getCameraDefinition_t original_getCameraDefinition = nullptr;

using updateCameraPitch_t = void(*)(uint64_t a1, uint64_t a2, RE::CameraObject* a3, RE::DeltaTimeObject* a4);
updateCameraPitch_t original_updateCameraPitch = nullptr;

// This gets called every frame in original_updateCamera
static void detour_updateCameraPitch(uint64_t a1, uint64_t a2, RE::CameraObject* a3, RE::DeltaTimeObject* a4)
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

void installHooks()
{
	if (MH_Initialize() != MH_OK) {
		std::cout << "Failed to initialize MinHook." << std::endl;
		return;
	}

	bool success = true;

	uintptr_t start, end;
	success &= getModuleRange(start, end);
	success &= installUpdateCameraHook(start, end);
	success &= installCalculatePitchHook(start, end);


	auto callAddr = findPattern(
		"48 8B 05 ?? ?? ?? ?? 80 B8 32 13 00 00 00 74 07",
		start,
		end
	);
	original_getCameraDefinition = reinterpret_cast<getCameraDefinition_t>(callAddr);
	std::cout << "getCameraDefinition found successfully." << std::endl;


	callAddr = findPattern(
		"E8 ?? ?? ?? ?? 48 8B 46 70 4C 8D 45 90 0F 28 46 30",
		start,
		end
	);
	auto relativeOffset = *reinterpret_cast<int32_t*>(callAddr + 1);
	auto targetAddr = callAddr + 5 + relativeOffset;
	success &= installHook((void*)targetAddr, &detour_updateCameraPitch, (void**)&original_updateCameraPitch, "updateCameraPitch");


	if (success) {
		std::cout << "All hooks installed successfully." << std::endl;
	}
	else {
		std::cout << "Some hooks failed to install." << std::endl;
		MH_Uninitialize();
	}
}