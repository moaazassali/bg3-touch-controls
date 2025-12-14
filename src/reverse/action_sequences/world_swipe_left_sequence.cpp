#include <array>

#include <reverse/action_request.h>


// Refer to world_swipe_right_sequence.cpp for more details.
namespace reverse {
	std::array<ActionRequest, 2> getWorldSwipeLeftSequence(float scaledValue, float rawValue, bool isPressed) {
		return {
			// CameraRotateMouseRight
			ActionRequest
			{
				.action = nullptr,
				.playerIndex = 0,
				.actionId = 110,
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
				.inputId = 129,
			},
			ActionRequest
			{
				.action = nullptr,
				.playerIndex = 0,
				.actionId = 110,
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
				.inputId = 129,
			},
		};
	}
}