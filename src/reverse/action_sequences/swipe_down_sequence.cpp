#include <array>

#include <reverse/action_request.h>


// Refer to swipe_up_sequence.cpp for more details.
// We need the actions triggered by mouse wheel up here.
// This is almost identical to the swipe up sequence except the actions triggered are the opposite ones.
// The inputId is 0x87 for mouse wheel up instead of 0x88 for mouse wheel down.
// The relevant actions is 0x5E = UIScrollUp instead of 0x5F = UIScrollDown.

namespace reverse {
	std::array<ActionRequest, 4> getSwipeDownSequence(float strength) {
		// pressValue and rawValue are never negative
		strength = std::abs(strength);
		return {
			ActionRequest
			{
				.action = nullptr,
				.playerIndex = 0,
				.actionId = 0x5E,
				.deviceId = 1,
				.playerIndex_dup1 = 0,
				.actionState = 1,
				.prevPressValue = 0.0f,
				.prevRawValue = 0.0f,
				.prevIsPressed = false,
				.currentPressValue = strength,
				.currentRawValue = strength,
				.currentIsPressed = true,
				.deviceId_dup1 = 1,
				.inputId = 0x87,
			},
			ActionRequest
			{
				.action = nullptr,
				.playerIndex = 0,
				.actionId = 0x5E,
				.deviceId = 1,
				.playerIndex_dup1 = 0,
				.actionState = 4,
				.prevPressValue = 0.0f,
				.prevRawValue = 0.0f,
				.prevIsPressed = false,
				.currentPressValue = strength,
				.currentRawValue = strength,
				.currentIsPressed = true,
				.deviceId_dup1 = 1,
				.inputId = 0x87,
			},
			ActionRequest
			{
				.action = nullptr,
				.playerIndex = 0,
				.actionId = 0x5E,
				.deviceId = 1,
				.playerIndex_dup1 = 0,
				.actionState = 2,
				.prevPressValue = strength,
				.prevRawValue = strength,
				.prevIsPressed = true,
				.currentPressValue = 0.0f,
				.currentRawValue = 0.0f,
				.currentIsPressed = false,
				.deviceId_dup1 = 1,
				.inputId = 0x87,
			},
			ActionRequest
			{
				.action = nullptr,
				.playerIndex = 0,
				.actionId = 0x5E,
				.deviceId = 1,
				.playerIndex_dup1 = 0,
				.actionState = 4,
				.prevPressValue = strength,
				.prevRawValue = strength,
				.prevIsPressed = true,
				.currentPressValue = 0.0f,
				.currentRawValue = 0.0f,
				.currentIsPressed = false,
				.deviceId_dup1 = 1,
				.inputId = 0x87,
			},
		};
	}
}
