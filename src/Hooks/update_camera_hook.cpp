#include <iostream>

#include "reverse/Camera.h"
#include "pending_camera_updates.h"
#include "game_bindings.h"
#include "reverse/functions.h"


reverse::updateCamera_t original_updateCamera = nullptr;


// Based on manual testing, this is called on every frame and is tied to the frame rate.
static void detour_updateCamera(uint64_t a1, uint64_t a2, reverse::DeltaTimeObject* a3, reverse::UnkObject* a4)
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

		const auto xPanDelta = g_pendingCameraUpdates.xPanDelta.exchange(0.0f);
		const auto yPanDelta = g_pendingCameraUpdates.yPanDelta.exchange(0.0f);
		const auto zoomDelta = g_pendingCameraUpdates.zoomDelta.exchange(0.0f);
		const auto isPanningOrZooming = xPanDelta != 0.0f || yPanDelta != 0.0f || zoomDelta != 0.0f;

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