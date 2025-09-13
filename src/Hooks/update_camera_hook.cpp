#include <iostream>

#include "RE/Camera.h"
#include "pending_camera_updates.h"
#include "game_bindings.h"
#include "reverse/functions.h"


reverse::updateCamera_t original_updateCamera = nullptr;


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

		const auto xPanDelta = g_pendingCameraUpdates.xPanDelta.exchange(0.0f);
		const auto yPanDelta = g_pendingCameraUpdates.yPanDelta.exchange(0.0f);
		const auto zoomDelta = g_pendingCameraUpdates.zoomDelta.exchange(0.0f);
		const auto isPanningOrZooming = xPanDelta != 0.0f && yPanDelta != 0.0f && zoomDelta != 0.0f;

		if (isPanningOrZooming && cam->followPlayer) {
			const float PAN_DOMINANCE_FACTOR = 10.0f;
			const float MIN_PAN_THRESHOLD = 1.5f;
			const float MIN_ZOOM_THRESHOLD = 0.5f;

			float translationMagnitude = sqrt(
				xPanDelta * xPanDelta +
				yPanDelta * yPanDelta
			);
			float expansionMagnitude = abs(zoomDelta);

			auto isPanningIntent = false;
			if (expansionMagnitude < MIN_ZOOM_THRESHOLD && translationMagnitude > MIN_PAN_THRESHOLD) {
				// Barely any zoom happening, so check if panning is significant enough.
				isPanningIntent = true;
			}
			else if (translationMagnitude > expansionMagnitude * PAN_DOMINANCE_FACTOR) {
				// Zoom is happening, so check if panning is significantly stronger than zooming.
				isPanningIntent = true;
			}

			if (isPanningIntent) {
				// If we are panning, we disable followPlayer to allow free movement.
				// If we are zooming only, we keep followPlayer enabled to keep the player centered.
				cam->followPlayer = false;
			}
			else {
				cam->zoomDelta = zoomDelta;
			}
		}
		else {
			cam->horizontalPanDelta = xPanDelta;
			cam->verticalPanDelta = yPanDelta;
			cam->zoomDelta = zoomDelta;
		}
	}
	original_updateCamera(a1, a2, a3, a4);
}


bool installUpdateCameraHook(uintptr_t start, uintptr_t end)
{
	uintptr_t callAddr = findPattern(
		"E8 ?? ?? ?? ?? 48 8D 8D F8 04 00 00 E8 ?? ?? ?? ?? E9 AF FD FF FF",
		start,
		end
	);
	// Calculate the target function address from the CALL instruction
	int32_t relativeOffset = *reinterpret_cast<int32_t*>(callAddr + 1);
	uintptr_t targetAddr = callAddr + 5 + relativeOffset;
	return installHook((void*)targetAddr, &detour_updateCamera, (void**)&original_updateCamera, "updateCamera");
}