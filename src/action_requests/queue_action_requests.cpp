#include <iostream>

#include "game_bindings.h"
#include "reverse/input_manager.h"
#include "action_requests.h"
#include "utils.h"


using processUnifiedInputQueue = void(*)(reverse::InputManager* a1, uint64_t a2);
processUnifiedInputQueue original_processUnifiedInputQueue = nullptr;

namespace reverse {
	InputManager* g_inputManager = nullptr;
}


// This is called every frame to handle input
// a1 is a pretty big "InputManager" singleton struct, which holds a lot of states, queues, etc...
// processUnifiedInputEvent a1 is also InputManager*, so why not grab it from there instead of hooking
// another function? processUnifiedInputEvent is called by processUnifiedInputQueue only when needed,
// so our input manager pointer will be null initially unless we make sure to send some "initial" input
// before doing anything relies on input manager.
static void detour_processUnifiedInputQueue(reverse::InputManager* a1, uint64_t a2)
{
	if (!reverse::g_inputManager) {
		reverse::g_inputManager = a1;
	}
	original_processUnifiedInputQueue(a1, a2);
}


bool installProcessUnifiedInputQueueHook(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_processUnifiedInputQueue = "48 89 54 24 10 48 89 4C 24 08 55 41 56 48 8D AC";
	uintptr_t matchAddr = findPattern(pattern_processUnifiedInputQueue, start, end);
	return installHook((void*)matchAddr, &detour_processUnifiedInputQueue, (void**)&original_processUnifiedInputQueue, "processUnifiedInputQueue");
}


// This takes a queue from the action requests queues list retrieved via getActionRequestQueuesList
// and pushes a new action request to it.
using pushToActionRequestQueue_t = reverse::ActionRequest* (*)(void* a1, reverse::ActionRequest* a2);
pushToActionRequestQueue_t original_pushToActionRequestQueue = nullptr;

static bool enableDebugPushToActionRequestQueue = false;


static reverse::ActionRequest* detour_pushToActionRequestQueue(void* a1, reverse::ActionRequest* a2)
{
	if (enableDebugPushToActionRequestQueue) {
		static uint64_t counter = 0;

		if (a2 && a2->action) {
			std::string actionName;
			if (a2->action->name && a2->action->nameLen > 0) {
				actionName.assign(a2->action->name, static_cast<size_t>(a2->action->nameLen));
			}

			if (a2->actionId != -1) {
				std::cout << counter << " - ";
				printActionRequestDetails(a2);
			}

			counter++;
		}
	}
	return original_pushToActionRequestQueue(a1, a2);
}


bool installPushToActionRequestQueueHook(uintptr_t start, uintptr_t end, bool enableDebug)
{
	constexpr const char* pattern_pushToActionRequestQueue = "48 89 5C 24 08 57 48 83 EC 20 48 8B D9 48 8B FA 8B 49 08 8B 43 0C 3B C8 7F 16 FF C0 8D 14 09 3B D0 0F 4C D0 3B CA 7D 08 48 8B CB E8 ?? ?? ?? ?? 48 63 4B 0C 8D 41 01 89 43 0C 48 8B";
	uintptr_t matchAddr = findPattern(pattern_pushToActionRequestQueue, start, end);
	if (!matchAddr) {
		printPatternNotFoundError(pattern_pushToActionRequestQueue, "pushToActionRequestQueue");
		return false;
	}
	if (enableDebug) {
		enableDebugPushToActionRequestQueue = enableDebug;
		return installHook((void*)matchAddr, &detour_pushToActionRequestQueue, (void**)&original_pushToActionRequestQueue, "pushToActionRequestQueueHook");
	}
	// We only need a reference to the original function if debugging is disabled
	original_pushToActionRequestQueue = (pushToActionRequestQueue_t)matchAddr;
	return true;
}


// This function retrieves the struct that holds a list of all action request queues.
// a1 is the address of what seems to be a list of action buffers
// it is at offset 0x40 of the InputManager singleton instance.
// a2 is the player index (useful for local co-op).
// 0 = player 1 (i.e. when playing solo or as the first player in local co-op)
using getActionRequestsQueuesList_t = void* (*)(void* a1, uint8_t* a2);
getActionRequestsQueuesList_t original_getActionRequestsQueuesList = nullptr;


bool installGetActionRequestsQueuesListHook(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_getActionRequestsQueuesList = "40 53 56 57 48 83 EC 20 48 63 71 2C 4C 8B C2 48 8D 54 24 40 48 8B D9 E8 ?? ?? ?? ?? 8B 53 28 48 8D 7B 30 3B 53 38 7E 2A";
	uintptr_t matchAddr = findPattern(pattern_getActionRequestsQueuesList, start, end);
	if (!matchAddr) {
		printPatternNotFoundError(pattern_getActionRequestsQueuesList, "getActionRequestsQueuesList");
		return false;
	}
	original_getActionRequestsQueuesList = (getActionRequestsQueuesList_t)matchAddr;
	return true;
}