#include <array>

#include "action_requests.h"


std::array<ActionRequest, 1> getTwoContactDragUpActions(float scaledValue, float rawValue) {
	return {
		ActionRequest
		{
			.action = nullptr,
			.playerIndex = 0,
			.actionId = 99, // CameraBackward
			.deviceId = 1, // keyboard
			.playerIndex_dup1 = 0,
			.actionState = 4,
			.prevPressValue = 0,
			.prevRawValue = 0,
			.prevIsPressed = 0,
			.currentPressValue = scaledValue,
			.currentRawValue = rawValue,
			.currentIsPressed = scaledValue > 0 ? true : false,
			.deviceId_dup1 = 1,
			.inputId = 88, // down arrow key
		},
	};
}


std::array<ActionRequest, 1> getTwoContactDragDownActions(float scaledValue, float rawValue) {
	return {
		ActionRequest
		{
			.action = nullptr,
			.playerIndex = 0,
			.actionId = 100, // CameraForward
			.deviceId = 1, // keyboard
			.playerIndex_dup1 = 0,
			.actionState = 4,
			.prevPressValue = 0,
			.prevRawValue = 0,
			.prevIsPressed = 0,
			.currentPressValue = scaledValue,
			.currentRawValue = rawValue,
			.currentIsPressed = scaledValue > 0 ? true : false,
			.deviceId_dup1 = 1,
			.inputId = 89, // up arrow key
		},
	};
}


std::array<ActionRequest, 1> getTwoContactDragRightActions(float scaledValue, float rawValue) {
	return {
		ActionRequest
		{
			.action = nullptr,
			.playerIndex = 0,
			.actionId = 101, // CameraLeft
			.deviceId = 1, // keyboard
			.playerIndex_dup1 = 0,
			.actionState = 4,
			.prevPressValue = 0,
			.prevRawValue = 0,
			.prevIsPressed = 0,
			.currentPressValue = scaledValue,
			.currentRawValue = rawValue,
			.currentIsPressed = scaledValue > 0 ? true : false,
			.deviceId_dup1 = 1,
			.inputId = 87, // left arrow key
		},
	};
}


std::array<ActionRequest, 1> getTwoContactDragLeftActions(float scaledValue, float rawValue) {
	return {
		ActionRequest
		{
			.action = nullptr,
			.playerIndex = 0,
			.actionId = 102, // CameraRight
			.deviceId = 1, // keyboard
			.playerIndex_dup1 = 0,
			.actionState = 4,
			.prevPressValue = 0,
			.prevRawValue = 0,
			.prevIsPressed = 0,
			.currentPressValue = scaledValue,
			.currentRawValue = rawValue,
			.currentIsPressed = scaledValue > 0 ? true : false,
			.deviceId_dup1 = 1,
			.inputId = 86, // right arrow key
		},
	};
}