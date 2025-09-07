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


using calculatePitch_t = float(*)(RE::CameraObject* a1, uint8_t a2, uint8_t a3);
calculatePitch_t original_calculatePitch = nullptr;

using getCameraDefinition_t = RE::CameraDefinition* (*)(RE::CameraObject* a1);
getCameraDefinition_t original_getCameraDefinition = nullptr;

using updateCameraPitch_t = void(*)(uint64_t a1, uint64_t a2, RE::CameraObject* a3, RE::DeltaTimeObject* a4);
updateCameraPitch_t original_updateCameraPitch = nullptr;

// This gets called every time in original_updateCamera when the camera.
// It also gets called in other places but we only care about the one in original_updateCamera.
// We have to constantly return the desired pitch here because the original calculatePitch function
// disregards any previous pitch changes and just uses some global values.
// No need to call original function because it doesn't do any state updates.
// It just returns a float based on currentPitch and some states/global values.
static float detour_calculatePitch(RE::CameraObject* a1, uint8_t a2, uint8_t a3)
{
	// We will use this to update the pitch and keep returning it because it is the value
	// that we want the pitch to remain at.
	static float pitch = 34.5f;

	constexpr float inertiaDecayFactor = 5.0f;

	// We need to update the pitch value based on any pending pitchDelta. This is similar to how we handle
	// angleDelta but we have to take care of updating the pitch value itself too.
	if (g_pendingCameraUpdates.pitchInertiaState.load() == InertiaState::InertiaStart) {
		const auto pitchDelta = g_pendingCameraUpdates.pitchDelta.load();
		pitch = std::clamp(pitch + pitchDelta, 5.0f, 85.55f);
		// new pitchDelta based on inertia decay factor
		// assuming 60fps for now since we don't have deltaTimeSeconds here
		const auto newPitchDelta = pitchDelta * (1 - inertiaDecayFactor * 0.01667f);
		g_pendingCameraUpdates.pitchDelta.store(newPitchDelta);
		g_pendingCameraUpdates.pitchInertiaState.store(InertiaState::InInertia);
	}
	else if (g_pendingCameraUpdates.pitchInertiaState.load() == InertiaState::InInertia) {
		const auto pitchDelta = g_pendingCameraUpdates.pitchDelta.load();
		if (std::abs(pitchDelta) < 0.1f) {
			g_pendingCameraUpdates.pitchDelta.store(0.0f);
			g_pendingCameraUpdates.pitchInertiaState.store(InertiaState::NotInInertia);
		}
		pitch = std::clamp(pitch + pitchDelta, 5.0f, 85.55f);
		// new pitchDelta based on inertia decay factor
		const auto newPitchDelta = pitchDelta * (1 - inertiaDecayFactor * 0.01667f);
		g_pendingCameraUpdates.pitchDelta.store(newPitchDelta);
	}
	else {
		pitch = std::clamp(pitch + g_pendingCameraUpdates.pitchDelta.exchange(0.0f), 5.0f, 85.55f);
	}
	return pitch;
}

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


	auto callAddr = findPattern(
		"E8 ?? ?? ?? ?? 80 BF 4C 01 00 00 00",
		start,
		end
	);
	auto relativeOffset = *reinterpret_cast<int32_t*>(callAddr + 1);
	auto targetAddr = callAddr + 5 + relativeOffset;
	success &= installHook((void*)targetAddr, &detour_calculatePitch, (void**)&original_calculatePitch, "calculatePitch");


	callAddr = findPattern(
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
	relativeOffset = *reinterpret_cast<int32_t*>(callAddr + 1);
	targetAddr = callAddr + 5 + relativeOffset;
	success &= installHook((void*)targetAddr, &detour_updateCameraPitch, (void**)&original_updateCameraPitch, "updateCameraPitch");


	if (success) {
		std::cout << "All hooks installed successfully." << std::endl;
	}
	else {
		std::cout << "Some hooks failed to install." << std::endl;
		MH_Uninitialize();
	}
}