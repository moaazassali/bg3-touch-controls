#include <iostream>
#include <chrono>

#include "reverse/functions.h"
#include "action_requests.h"
#include "game_bindings.h"
#include "utils.h"


void* detour_filterPreFinalActionsQueue(void* a1, void* a2, void* a3) {
	if (!a3) return original_filterPreFinalActionsQueue(a1, a2, a3);
	uintptr_t queueStart = *(uintptr_t*)a3;
	int32_t count = *(int32_t*)((uintptr_t)a3 + 0x0C);

	if (count <= 0) {
		return original_filterPreFinalActionsQueue(a1, a2, a3);
	}

	std::cout << "Processing Action Requests Queue with " << count << " items." << std::endl;
	uintptr_t curr = queueStart;

	for (int i = 0; i < count; ++i) {
		ActionRequest* req = (ActionRequest*)curr;

		if (req && req->action && req->actionId != -1) {
			std::cout << "F#" << i << " - ";
			printActionRequestDetails(req);
		}

		// Manually advance by the struct size (ActionRequest size / 56 bytes)
		curr += 56;
	}

	return original_filterPreFinalActionsQueue(a1, a2, a3);
}


bool installFilterPreFinalActionsQueueHook(uintptr_t start, uintptr_t end)
{
	uintptr_t address = start + reverse::offset_filterPreFinalActionsQueue;
	if (!installHook((LPVOID)address, (LPVOID)detour_filterPreFinalActionsQueue, (LPVOID*)&original_filterPreFinalActionsQueue, "filterPreFinalActionsQueue")) {
		return false;
	}
	return true;
}