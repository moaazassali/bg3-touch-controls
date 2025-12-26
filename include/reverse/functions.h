#pragma once
#include <cstdint>
#include <SDL.h>

#include "reverse/Camera.h"
#include "reverse/input_manager.h"
#include "reverse/action_request.h"


namespace reverse {
	// Adapted from BG3_NativeCameraTweaks:
	// https://github.com/ersh1/BG3_NativeCameraTweaks/blob/876f2da98ede96d3db2884ffd9fb25687ada5ff1/src/Hooks.h
	using updateCamera_t = void(*)(uint64_t a1, uint64_t a2, reverse::DeltaTimeObject* a3, reverse::UnkObject* a4);
	constexpr const char* pattern_updateCamera = "E8 ?? ?? ?? ?? 48 8D 8D F8 04 00 00 E8 ?? ?? ?? ?? E9 AF FD FF FF";

	// Adapted from BG3_NativeCameraTweaks
	using calculatePitch_t = float(*)(reverse::CameraObject* a1, uint8_t a2, uint8_t a3);
	constexpr const char* pattern_calculatePitch = "E8 ?? ?? ?? ?? 80 BF 4C 01 00 00 00";

	// Adapted from BG3_NativeCameraTweaks
	using getCameraDefinition_t = reverse::CameraDefinition* (*)(reverse::CameraObject* a1);
	constexpr const char* pattern_getCameraDefinition = "48 8B 05 ?? ?? ?? ?? 80 B8 32 13 00 00 00 74 07";

	// Adapted from BG3_NativeCameraTweaks
	using updateCameraPitch_t = void(*)(uint64_t a1, uint64_t a2, reverse::CameraObject* a3, reverse::DeltaTimeObject* a4);
	constexpr const char* pattern_updateCameraPitch = "E8 ?? ?? ?? ?? 48 8B 46 70 4C 8D 45 90 0F 28 46 30";

	// Unknown return type and parameters
	// Called repeatedly (not sure about interval) when using controller to check if character should be moved
	// NOPing this stops character movement via controller left stick input
	// First called on world load
	using moveCharacter_t = void* (*)(void* a1, void* a2, void* a3);
	constexpr const char* pattern_moveCharacter = "E8 ?? ?? ?? ?? F3 0F 10 55 ?? F3 0F 10 45 ?? F3 0F 10 4D ?? F3 0F 10 35";

	// This takes a queue range from the action requests buffer retrieved via getActionRequestsBuffer
	// and processes all action requests in it.
	// It does some filtering based on the current game state (like whether the UI is open or not)
	// and then picks the most relevant action, which it pushes to the final action queue to be executed.
	// a2 is a struct that this method fills out.
	// a3 is a struct that holds two pointers to the start and end of the action requests queue.
	using processActionRequestsQueue_t = void* (*)(InputManager* a1, void* a2, void* a3, uint8_t a4);
	constexpr const uintptr_t offset_processActionRequestsQueue = 0x40C41C0;

	// I am not very sure what this does yet, but it might be the only function that reads from the
	// final filtered action requests queue to dispatch the actions to the game objects.
	// a1 is a pointer to some massive singleton. The byte at 0x240 indicates whether the cursor is over UI.
	// a2 might be the cursor coordinates.
	using unk_consumesFilteredActionsQueue_t = void(*)(void* a1, double* a2);
	constexpr const uintptr_t offset_unk_consumesFilteredActionsQueue = 0x120C620;

	// This also consumes some pre-final? filtered actions queue and dispatches them to a more final queue.
	// a1 is some big singleton pointer.
	// a2 is some struct this function fills out which ends up being an action request + maybe some flag and a pointer
	// a3 is a pointer to action requests queue struct
	// This can be used for debugging to check what actions end up in this pre-final queue.
	using filterPreFinalActionsQueue_t = void* (*)(void* a1, void* a2, void* a3);
	constexpr const uintptr_t offset_filterPreFinalActionsQueue = 0x12108F0;
}