#include <array>

#include "action_requests.h"

// When the mouse is moved to the right, the game triggers several actions.
// The one we care about for rotating the camera so far is CameraRotateMouseLeft (action ID 109).
// For some reason, I always see two action requests triggered per mouse move event.
// The first one with actual values and the second one with zeroed values.
// I am not sure if we really need the second one. Maybe it improves responsiveness? Maybe it makes it worse? Not sure...
namespace reverse {
	std::array<ActionRequest, 2> getWorldSwipeRightSequence(float scaledValue, float rawValue, bool isPressed) {
		return {
			// CameraRotateMouseLeft
			ActionRequest
			{
				.action = nullptr,
				.playerIndex = 0,
				.actionId = 109,
				.deviceId = 1,
				.playerIndex_dup1 = 0,
				.actionState = 4,
				.prevPressValue = 0,
				.prevRawValue = 0,
				.prevIsPressed = 0,
				.currentPressValue = scaledValue,
				.currentRawValue = rawValue,
				.currentIsPressed = isPressed,
				.deviceId_dup1 = 1,
				.inputId = 131,
			},
			ActionRequest
			{
				.action = nullptr,
				.playerIndex = 0,
				.actionId = 109,
				.deviceId = 1,
				.playerIndex_dup1 = 0,
				.actionState = 4,
				.prevPressValue = scaledValue,
				.prevRawValue = rawValue,
				.prevIsPressed = isPressed,
				.currentPressValue = 0,
				.currentRawValue = 0,
				.currentIsPressed = 0,
				.deviceId_dup1 = 1,
				.inputId = 131,
			},
		};
	}
}