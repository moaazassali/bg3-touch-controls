#include <windows.h>
#include <iostream>
#include <Psapi.h>
#include <vector>
#include <MinHook.h>
#include "RE/Camera.h"
#include "pending_camera_updates.h"
#include <algorithm>

// Taken from BG3_NativeCameraTweaks:
// https://github.com/ersh1/BG3_NativeCameraTweaks/blob/876f2da98ede96d3db2884ffd9fb25687ada5ff1/src/Hooks.h
using updateCamera_t = void(*)(uint64_t a1, uint64_t a2, RE::DeltaTimeObject* a3, RE::UnkObject* a4);
updateCamera_t original_updateCamera = nullptr;

using calculatePitch_t = float(*)(RE::CameraObject* a1, uint8_t a2, uint8_t a3);
calculatePitch_t original_calculatePitch = nullptr;

using getCameraDefinition_t = RE::CameraDefinition* (*)(RE::CameraObject* a1);
getCameraDefinition_t original_getCameraDefinition = nullptr;

using updateCameraPitch_t = void(*)(uint64_t a1, uint64_t a2, RE::CameraObject* a3, RE::DeltaTimeObject* a4);
updateCameraPitch_t original_updateCameraPitch = nullptr;

// Simple pattern scanner created by an LLM
static uintptr_t findPattern(const char* pattern, uintptr_t start, uintptr_t end)
{
	std::vector<int> bytes;
	const char* p = pattern;
	while (*p)
	{
		if (*p == ' ')
		{
			p++;
			continue;
		}
		if (*p == '?')
		{
			bytes.push_back(-1); // Wildcard
			p += 2;
		}
		else
		{
			char byte[3] = { *p, *(p + 1), 0 };
			bytes.push_back(strtol(byte, nullptr, 16));
			p += 2;
		}
	}

	for (uintptr_t i = start; i < end - bytes.size(); ++i)
	{
		bool found = true;
		for (size_t j = 0; j < bytes.size(); ++j)
		{
			if (bytes[j] != -1 && *(uint8_t*)(i + j) != bytes[j])
			{
				found = false;
				break;
			}
		}
		if (found)
			return i;
	}
	return 0;
}

// Helper to get module range created by an LLM
static bool getModuleRange(uintptr_t& start, uintptr_t& end)
{
	HMODULE hModule = GetModuleHandle(NULL);
	if (!hModule) {
		std::cout << "GetModuleHandle failed." << std::endl;
		return false;
	}

	MODULEINFO moduleInfo;
	if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(MODULEINFO))) {
		std::cout << "GetModuleInformation failed." << std::endl;
		return false;
	}

	start = reinterpret_cast<uintptr_t>(hModule);
	end = start + moduleInfo.SizeOfImage;
	return true;
}

static void handleAngleDeltaUpdate(RE::CameraObject* cam, float deltaTimeSeconds) {
	if (g_pendingCameraUpdates.angleInertiaState.load() == InertiaState::InertiaStart) {
		// On the first frame of inertia, we set any pending angleDelta to currentAngleDelta
		// This ensures that if the user swiped and then lifted their finger, the velocity
		// of the swipe is taken into account for the inertia.
		// In many cases, cam->currentAngleDelta will be 0.0f at this point because we already
		// consumed the angleDelta in the previous frame when not in inertia mode.
		// So we need to make sure we have the initial value to start the inertia from.
		// Otherwise, currentAngleDelta will start from 0 and do nothing.
		// If it was not 0, that means the user swiped again while inertia was ongoing, so we
		// still need to remove the previous angleDelta value and use the new one.
		cam->currentAngleDelta = g_pendingCameraUpdates.angleDelta.exchange(0.0f);
		g_pendingCameraUpdates.angleInertiaState.store(InertiaState::InInertia);
	}
	else if (g_pendingCameraUpdates.angleInertiaState.load() == InertiaState::InInertia) {
		constexpr float inertiaDecayFactor = 10.0f;
		// multiply by deltaTimeSeconds to make it frame rate independent
		cam->currentAngleDelta *= 1 - inertiaDecayFactor * deltaTimeSeconds;
		if (std::abs(cam->currentAngleDelta) < 0.1f) {
			cam->currentAngleDelta = 0.0f;
			g_pendingCameraUpdates.angleInertiaState.store(InertiaState::NotInInertia);
		}
	}
	else {
		// We will use the amount of angleDelta as a measure of how fast we want to rotate.
		// This makes sense since angleDelta is based on how far the user moved their finger
		// within a frame, which is a measure of speed.
		// I attempted adjusting camer angle directly first but it didn't work well.
		// The range of the angle field is from (-360, 360) according to testing.
		// But how do you represent a fast rotation with that and especially one that involves
		// possibly multiple full rotations in one frame (unlikely)?
		// Although this exact logic of trying to update angle was causing sudden camera jumps
		// for fast swipes and jittery behavior The rotation somehow didn't always change just 
		// the axis we want but seemed to affect axes?
		cam->currentAngleDelta = g_pendingCameraUpdates.angleDelta.exchange(0.0f);
	}
}

// Based on manual testing, this is called on every frame and is tied to the frame rate.
static void detour_updateCamera(uint64_t a1, uint64_t a2, RE::DeltaTimeObject* a3, RE::UnkObject* a4)
{
	// There are two CameraObject pointers in UnkObject. Only currentCameraObject seems to work.
	// Updating desiredCameraObject2 doesn't seem to have any effect.
	if (a4 && a4->currentCameraObject) {
		// In the future, to allow for dual input modes (e.g. gamepad + touch), we will probably
		// have to run this logic only if there are pending updates as to not override other input
		// modes especially if some calculations are based on current value + whatever amount the
		// user input adds.
		// For now we assume only one input mode (touch).
		auto cam = a4->currentCameraObject;

		handleAngleDeltaUpdate(cam, a3->deltaTimeSeconds);

		// Similar rationale to angleDelta above.
		cam->zoomDelta = g_pendingCameraUpdates.zoomDelta.exchange(0.0f);

		// Only disable followPlayer if there is some panning input.
		// The camera will unexpectedly stop following the player otherwise.
		// horizontalPanDelta and verticalPanDelta have no effect unless followPlayer is false.
		if (g_pendingCameraUpdates.xPanDelta.load() != 0.0f || g_pendingCameraUpdates.yPanDelta.load() != 0.0f) {
			cam->followPlayer = false;
		}

		// invert for natural feel. Add as a config.
		cam->horizontalPanDelta = -g_pendingCameraUpdates.xPanDelta.exchange(0.0f);
		cam->verticalPanDelta = -g_pendingCameraUpdates.yPanDelta.exchange(0.0f);
	}
	original_updateCamera(a1, a2, a3, a4);
}

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

	uintptr_t start, end;
	if (!getModuleRange(start, end)) {
		MH_Uninitialize();
		return;
	}
	uintptr_t callAddr = findPattern(
		"E8 ?? ?? ?? ?? 48 8D 8D F8 04 00 00 E8 ?? ?? ?? ?? E9 AF FD FF FF",
		start,
		end
	);
	// Calculate the target function address from the CALL instruction
	int32_t relativeOffset = *reinterpret_cast<int32_t*>(callAddr + 1);
	uintptr_t targetAddr = callAddr + 5 + relativeOffset;

	if (MH_CreateHook((void*)targetAddr, &detour_updateCamera, (void**)&original_updateCamera) != MH_OK)
	{
		std::cout << "Failed to create hook." << std::endl;
		MH_Uninitialize();
		return;
	}

	if (MH_EnableHook((void*)targetAddr) != MH_OK)
	{
		printf("Failed to enable hook\n");
		MH_Uninitialize();
		return;
	}

	std::cout << "UpdateCamera hook installed successfully." << std::endl;

	callAddr = findPattern(
		"E8 ?? ?? ?? ?? 80 BF 4C 01 00 00 00",
		start,
		end
	);
	relativeOffset = *reinterpret_cast<int32_t*>(callAddr + 1);
	targetAddr = callAddr + 5 + relativeOffset;
	if (MH_CreateHook((void*)targetAddr, &detour_calculatePitch, (void**)&original_calculatePitch) != MH_OK)
	{
		std::cout << "Failed to create calculatePitch hook." << std::endl;
		MH_Uninitialize();
		return;
	}
	if (MH_EnableHook((void*)targetAddr) != MH_OK)
	{
		printf("Failed to enable calculatePitch hook\n");
		MH_Uninitialize();
		return;
	}
	std::cout << "calculatePitch hook installed successfully." << std::endl;

	callAddr = findPattern(
		"48 8B 05 ?? ?? ?? ?? 80 B8 32 13 00 00 00 74 07",
		start,
		end
	);
	original_getCameraDefinition = reinterpret_cast<getCameraDefinition_t>(callAddr);
	std::cout << "getCameraDefinition hook installed successfully." << std::endl;

	callAddr = findPattern(
		"E8 ?? ?? ?? ?? 48 8B 46 70 4C 8D 45 90 0F 28 46 30",
		start,
		end
	);
	relativeOffset = *reinterpret_cast<int32_t*>(callAddr + 1);
	targetAddr = callAddr + 5 + relativeOffset;
	if (MH_CreateHook((void*)targetAddr, &detour_updateCameraPitch, (void**)&original_updateCameraPitch) != MH_OK)
	{
		std::cout << "Failed to create updateCameraPitch hook." << std::endl;
		MH_Uninitialize();
		return;
	}
	if (MH_EnableHook((void*)targetAddr) != MH_OK) {
		printf("Failed to enable updateCameraPitch hook\n");
		MH_Uninitialize();
		return;
	}
	std::cout << "updateCameraPitch hook installed successfully." << std::endl;
}