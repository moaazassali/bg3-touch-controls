#include <algorithm>

#include "game_bindings.h"
#include "utils.h"
#include "pending_camera_updates.h"


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


// Adapted from BG3_NativeCameraTweaks
using calculatePitch_t = float(*)(CameraObject* a1, uint8_t a2, uint8_t a3);
calculatePitch_t original_calculatePitch = nullptr;


// This gets called every time in original_updateCamera when the camera.
// It also gets called in other places but we only care about the one in original_updateCamera.
// We have to constantly return the desired pitch here because the original calculatePitch function
// disregards any previous pitch changes and just uses some global values.
// No need to call original function because it doesn't do any state updates.
// It just returns a float based on currentPitch and some states/global values.
static float detour_calculatePitch(CameraObject* a1, uint8_t a2, uint8_t a3)
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


bool installCalculatePitchHook(uintptr_t start, uintptr_t end)
{
	constexpr const char* calculatePitchPattern = "E8 ?? ?? ?? ?? 80 BF 4C 01 00 00 00";
	auto matchAddr = findPattern(calculatePitchPattern, start, end);
	// Calculate the target function address from the CALL instruction
	int32_t relativeOffset = *reinterpret_cast<int32_t*>(matchAddr + 1);
	uintptr_t targetAddr = matchAddr + 5 + relativeOffset;
	return installHook((void*)targetAddr, &detour_calculatePitch, (void**)&original_calculatePitch, "calculatePitch");
}