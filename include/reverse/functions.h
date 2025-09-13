#pragma once
#include <cstdint>

#include "RE/Camera.h"
#include "RE/InputManager.h"
#include "RE/ActionRequest.h"


namespace reverse {
	// Adapted from BG3_NativeCameraTweaks:
	// https://github.com/ersh1/BG3_NativeCameraTweaks/blob/876f2da98ede96d3db2884ffd9fb25687ada5ff1/src/Hooks.h
	using updateCamera_t = void(*)(uint64_t a1, uint64_t a2, RE::DeltaTimeObject* a3, RE::UnkObject* a4);
	constexpr const char* pattern_updateCamera = "E8 ?? ?? ?? ?? 48 8D 8D F8 04 00 00 E8 ?? ?? ?? ?? E9 AF FD FF FF";

	// Adapted from BG3_NativeCameraTweaks
	using calculatePitch_t = float(*)(RE::CameraObject* a1, uint8_t a2, uint8_t a3);
	constexpr const char* pattern_calculatePitch = "E8 ?? ?? ?? ?? 80 BF 4C 01 00 00 00";

	// Adapted from BG3_NativeCameraTweaks
	using getCameraDefinition_t = RE::CameraDefinition* (*)(RE::CameraObject* a1);
	constexpr const char* pattern_getCameraDefinition = "48 8B 05 ?? ?? ?? ?? 80 B8 32 13 00 00 00 74 07";

	// Adapted from BG3_NativeCameraTweaks
	using updateCameraPitch_t = void(*)(uint64_t a1, uint64_t a2, RE::CameraObject* a3, RE::DeltaTimeObject* a4);
	constexpr const char* pattern_updateCameraPitch = "E8 ?? ?? ?? ?? 48 8B 46 70 4C 8D 45 90 0F 28 46 30";


	// This is called every frame to handle input
	// a1 is a pretty big "InputManager" struct, which holds a lot of states, queues, etc...
	// It process the input events and dispatches a set of predefined actions
	// These actions can be mined from the game exe as they are associated with string literals
	using processInput_t = void(*)(InputManager* a1, uint64_t a2);
	constexpr const uintptr_t offset_processInput = 0x40C2CC0;

	// This function retrieves the raw action buffer that holds all actions that are mapped to the
	// input provided by the player.
	// So the actions in this buffer are all unfiltered and unprocessed.
	// A later stage will process these actions and filter them based on the current game state.
	// i.e. a mouse scroll up event maps to ui scroll and camera zoom, but if the ui is open
	// only the ui scroll action should be used, so the camera zoom action is filtered out.
	// a1 is the address of what seems to be a list of action buffers, one for game "context"/player.
	// it is at offset 0x40 of the InputManager singleton instance.
	// a2 is the current game "context" which is used for local co-op.
	// 0 = player 1 (i.e. when playing solo or as the first player in local co-op)
	using getActionRequestsBuffer_t = void* (*)(void* a1, uint8_t* a2);
	constexpr const uintptr_t offset_getActionRequestsBuffer = 0x40C79E0;

	// This takes a queue from the action requests buffer retrieved via getActionRequestsBuffer
	// and pushes a new action request to it.
	using pushToActionRequestsQueue_t = ActionRequest * (*)(void* a1, ActionRequest* a2);
	constexpr const uintptr_t offset_pushToActionRequestsQueue = 0x121C460;

	// This takes a queue range from the action requests buffer retrieved via getActionRequestsBuffer
	// and processes all action requests in it.
	// It does some filtering based on the current game state (like whether the UI is open or not)
	// and then picks the most relevant action, which it pushes to the final action queue to be executed.
	// a2 is a struct that this method fills out.
	// a3 is a struct that holds two pointers to the start and end of the action requests queue.
	using processActionRequestsQueue_t = void* (*)(InputManager* a1, void* a2, void* a3, uint8_t a4);
	constexpr const uintptr_t offset_processActionRequestsQueue = 0x40C41C0;
}