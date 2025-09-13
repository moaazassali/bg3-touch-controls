#include <algorithm>

#include "RE/Camera.h"
#include "pending_camera_updates.h"
#include "game_bindings.h"
#include "reverse/functions.h"


reverse::calculatePitch_t original_calculatePitch = nullptr;


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


bool installCalculatePitchHook(uintptr_t start, uintptr_t end)
{
	uintptr_t callAddr = findPattern(
		"E8 ?? ?? ?? ?? 80 BF 4C 01 00 00 00",
		start,
		end
	);
	// Calculate the target function address from the CALL instruction
	int32_t relativeOffset = *reinterpret_cast<int32_t*>(callAddr + 1);
	uintptr_t targetAddr = callAddr + 5 + relativeOffset;
	return installHook((void*)targetAddr, &detour_calculatePitch, (void**)&original_calculatePitch, "calculatePitch");
}