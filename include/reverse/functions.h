#pragma once
#include <cstdint>
#include <SDL.h>

#include "game_camera.h"


namespace reverse {
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
	// a1 is a pointer to the InputManager singleton.
	// a2 is a struct that this method fills out.
	// a3 is a struct that holds two pointers to the start and end of the action requests queue.
	using processActionRequestsQueue_t = void* (*)(void* a1, void* a2, void* a3, uint8_t a4);
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