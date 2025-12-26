#include <array>

#include "action_requests.h"


namespace reverse {
	std::array<ActionRequest, 1> getWorldSwipeToggleSequence(bool isStart) {
		uint8_t actionState = isStart ? 1 : 2;
		auto pressValue = isStart ? 1.0f : 0.0f;
		auto rawValue = isStart ? 1.0f : 0.0f;
		auto isPressed = isStart ? true : false;
		auto prevPressValue = isStart ? 0.0f : 1.0f;
		auto prevRawValue = isStart ? 0.0f : 1.0f;
		auto prevIsPressed = isStart ? false : true;

		return {
			// CameraToggleMouseRotate
			ActionRequest
			{
				.action = nullptr,
				.playerIndex = 0,
				.actionId = 98,
				.deviceId = 1,
				.playerIndex_dup1 = 0,
				.actionState = actionState,
				.prevPressValue = prevPressValue,
				.prevRawValue = prevRawValue,
				.prevIsPressed = prevIsPressed,
				.currentPressValue = pressValue,
				.currentRawValue = rawValue,
				.currentIsPressed = isPressed,
				.deviceId_dup1 = 1,
				.inputId = 124,
			},
		};
	}
}