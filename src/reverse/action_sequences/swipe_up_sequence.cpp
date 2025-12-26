#include <array>

#include "action_requests.h"


// When you scroll just once with the mouse scroll wheel, the following actions are triggered:
// 
// A raw input event is created with input id 0x88 and isPressed = true
// 0x19 = DefaultCameraZoomOut, queue 3, action state 4, event values
// 0x49 = WidgetScrollUp - queue 3, action state 4, event values
// 0x5F = UIScrollDown - queue 1, action state 1, event values
// 0x5F = UIScrollDown - queue 3, action state 4, event values
// 0x69 = CameraZoomOut - queue 3, action state 4, event values
// 0x75 = FreeCameraFovDec - queue 3, action state 4, event values
// 0x97 = PhotoCameraRotateDownAlt - queue 3, action state 4, event values
// 0xB6 = RotateItemLeft - queue 3, action state 4, event values
//
// [new buffer below bcz address is too far from the others]
// 0x5F = UIScrollDown - queue new, action state 1, event values
// [new buffer below bcz queue address is too far from the others]
// [despite action state being different, queue is the same]
// 0x18 = DefaultCameraZoomOut - queue new, action state 4, event values
//
// A raw input event is created with input id 0x88 and isPressed = false
// 0x18 = DefaultCameraZoomIn - queue 3, action state 4, dominant values
// 0x49 = WidgetScrollUp - queue 3, action state 4, dominant values
// 0x5F = UIScrollDown - queue 2, action state 2, dominant values
// 0x5F = UIScrollDown - queue 3, action state 4, dominant values
// 0x69 = CameraZoomOut - queue 3, action state 4, dominant values
// 0x75 = FreeCameraFovDec - queue 3, action state 4, dominant values
// 0x97 = PhotoCameraRotateDownAlt - queue 3, action state 4, dominant values
// 0xB6 = RotateItemLeft - queue 3, action state 4, dominant values
//
// [same new buffer below bcz address is too far from the others]
// 0x5F = UIScrollDown - queue new, action state 2, dominant values
// [same new buffer below bcz queue address is too far from the others]
// [despite action state being different, queue is the same]
// 0x18 = DefaultCameraZoomOut - queue new, action state 4, dominant values
//
//
// The new buffer corresponds to the final queue of actions which have been chosen
// given the game state I was in when testing (which was in a UI menu).
//
//
// Given this, we don't care about any of the camera actions because we don't want to
// trigger those on panning since we handle camera movement already.
// Item rotation might be interesting. I think that corresponds to the inspecting items
// and moving them around in the UI item inspection window. We should look into this
// and trigger those events on a left/right swipe instead.
// This leaves the UIScrollDown and WidgetScrollUp actions.
// I am not yet sure what WidgetScrollUp does so I will ignore it for now (maybe it's for
// the small UI windows like chest inventory? or maybe the on-screen skills UI?)
//
//
// The pressValue was 330.52386 and the rawValue was 5 when I sent a single SDL scroll wheel
// event with 5 scroll amount for the first event. The pressValue seemed to change though when
// I sent the same scroll event again? (needs more testing).
// The pressValue and rawValue for the second event triggered were both 0.

namespace reverse {
	std::array<ActionRequest, 4> getSwipeUpSequence(float strength) {
		// pressValue and rawValue are never negative
		strength = std::abs(strength);
		return {
			ActionRequest
			{
				.action = nullptr,
				.playerIndex = 0,
				.actionId = 0x5F,
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
				.inputId = 0x88,
			},
			ActionRequest
			{
				.action = nullptr,
				.playerIndex = 0,
				.actionId = 0x5F,
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
				.inputId = 0x88,
			},
			ActionRequest
			{
				.action = nullptr,
				.playerIndex = 0,
				.actionId = 0x5F,
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
				.inputId = 0x88,
			},
			ActionRequest
			{
				.action = nullptr,
				.playerIndex = 0,
				.actionId = 0x5F,
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
				.inputId = 0x88,
			},
		};
	}
}