#include <iostream>
#include <span>

#include "game_bindings.h"
#include "action_requests.h"
#include "utils.h"


// Action map node retrieved from the action map
struct ActionMapNode {
	ActionMapNode* next;
	uint32_t key;
	std::byte _pad_008[4];
	Action action;
};

struct InputManager {
	std::byte _pad_000[0x40];
	std::byte actionBufferManagerStuctStart;
	std::byte _pad_041[0x7F];
	uint32_t actionMapCount;
	uint32_t actionMapSize;
	ActionMapNode** actionMap;
	std::byte _pad_0D0[0x520];
	// More is known but not needed for now. Will add+document later.
};

using processUnifiedInputQueue = void(*)(InputManager* a1, uint64_t a2);
processUnifiedInputQueue original_processUnifiedInputQueue = nullptr;
InputManager* processUnifiedInputQueue_a1 = nullptr;


// This is called every frame to handle input
// a1 is a pretty big "InputManager" singleton struct, which holds a lot of states, queues, etc...
// processUnifiedInputEvent a1 is also InputManager*, so why not grab it from there instead of hooking
// another function? processUnifiedInputEvent is called by processUnifiedInputQueue only when needed,
// so our input manager pointer will be null initially unless we make sure to send some "initial" input
// before doing anything relies on input manager.
static void detour_processUnifiedInputQueue(InputManager* a1, uint64_t a2)
{
	if (a1) {
		processUnifiedInputQueue_a1 = reinterpret_cast<InputManager*>(a1);
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
using pushToActionRequestQueue_t = ActionRequest* (*)(void* a1, ActionRequest* a2);
pushToActionRequestQueue_t original_pushToActionRequestQueue = nullptr;

static bool enableDebugPushToActionRequestQueue = false;


static ActionRequest* detour_pushToActionRequestQueue(void* a1, ActionRequest* a2)
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


static Action* getActionById(ActionMapNode** actionMap, uint32_t actionMapSize, uint32_t actionId) {
	if (!actionMap || actionMapSize <= 0) {
		return nullptr;
	}

	size_t bucketIndex = actionId % actionMapSize;
	ActionMapNode* currentNode = actionMap[bucketIndex];

	// handle collisions using linked list traversal
	while (currentNode) {
		if (currentNode->action.actionId == actionId) {
			return &currentNode->action;
		}
		currentNode = currentNode->next;
	}

	return nullptr;
}


void populateActionPointers(std::span<ActionRequest> seq) {
	EnterCriticalSection((CRITICAL_SECTION*)processUnifiedInputQueue_a1);
	for (ActionRequest& req : seq) {
		req.action = getActionById(processUnifiedInputQueue_a1->actionMap, processUnifiedInputQueue_a1->actionMapSize, req.actionId);
	}
	LeaveCriticalSection((CRITICAL_SECTION*)processUnifiedInputQueue_a1);
}


void pushActionRequestsToQueue(std::span<ActionRequest> seq, uint8_t playerIndex) {
	EnterCriticalSection((CRITICAL_SECTION*)processUnifiedInputQueue_a1);
	void* actionRequestsBuffer = original_getActionRequestsQueuesList(reinterpret_cast<void*>(&processUnifiedInputQueue_a1->actionBufferManagerStuctStart), &playerIndex);
	auto q1 = actionRequestsBuffer;
	auto q2 = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(actionRequestsBuffer) + 0x10);
	auto q3 = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(actionRequestsBuffer) + 0x20);

	for (ActionRequest& req : seq) {
		if (req.actionState == 1) {
			original_pushToActionRequestQueue(q1, &req);
		}
		else if (req.actionState == 2) {
			original_pushToActionRequestQueue(q2, &req);
		}
		else if (req.actionState == 4) {
			original_pushToActionRequestQueue(q3, &req);
		}
	}
	LeaveCriticalSection((CRITICAL_SECTION*)processUnifiedInputQueue_a1);
}