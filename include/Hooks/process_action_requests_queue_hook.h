#pragma once
#include <cstdint>

#include "RE/InputManager.h"

// This takes a queue range from the action requests buffer retrieved via getActionRequestsBuffer
// and processes all action requests in it.
// It does some filtering based on the current game state (like whether the UI is open or not)
// and then picks the most relevant action, which it pushes to the final action queue to be executed.
// a2 is a struct that this method fills out.
// a3 is a struct that holds two pointers to the start and end of the action requests queue.
using processActionRequestsQueue_t = void* (*)(InputManager* a1, void* a2, void* a3, uint8_t a4);

extern processActionRequestsQueue_t original_processActionRequestsQueue;

bool installProcessActionRequestsQueueHook(uintptr_t start, uintptr_t end);