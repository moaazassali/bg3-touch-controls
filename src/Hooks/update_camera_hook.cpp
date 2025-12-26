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
		cam->zoomDelta = g_pendingCameraUpdates.zoomDelta.exchange(0.0f);
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