#include <iostream>
#include <chrono>

#include "reverse/functions.h"
#include "reverse/action_request.h"
#include "game_bindings.h"


static uint64_t counter = 0;


static void printActionRequestDetails(reverse::ActionRequest* a2) {
	if (a2 && a2->action) {
		std::string actionName;
		if (a2->action->name && a2->action->nameLen > 0) {
			actionName.assign(a2->action->name, static_cast<size_t>(a2->action->nameLen));
		}
		std::cout
			<< (actionName.empty() ? "null" : actionName)
			<< ", ID: " << a2->actionId
			<< ", Device: " << a2->deviceId
			<< ", State: " << static_cast<int>(a2->actionState)
			<< ", Scaled: " << a2->currentPressValue
			<< ", Raw: " << a2->currentRawValue
			<< ", Pressed: " << static_cast<int>(a2->currentIsPressed)
			<< ", Prev press: " << a2->prevPressValue
			<< ", Prev raw: " << a2->prevRawValue
			<< ", Prev pressed: " << static_cast<int>(a2->prevIsPressed)
			<< ", DeviceDup: " << a2->deviceId_dup1
			<< ", InputId: " << a2->inputId
			<< ", PlayerIdx: " << static_cast<int>(a2->playerIndex)
			<< ", PlayerIdxDup: " << static_cast<int>(a2->playerIndex_dup1)
			<< std::endl;
	}
}


reverse::ActionRequest* detour_pushToActionRequestsQueue(void* a1, reverse::ActionRequest* a2)
{
	// print action request details for debugging
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

	return original_pushToActionRequestsQueue(a1, a2);
}


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
		reverse::ActionRequest* req = (reverse::ActionRequest*)curr;

		if (req && req->action && req->actionId != -1) {
			std::cout << "F#" << i << " - ";
			printActionRequestDetails(req);
		}

		// Manually advance by the struct size (ActionRequest size / 56 bytes)
		curr += 56;
	}

	return original_filterPreFinalActionsQueue(a1, a2, a3);
}

bool installPushToActionRequestsQueueHook(uintptr_t start, uintptr_t end)
{
	uintptr_t address = start + reverse::offset_pushToActionRequestsQueue;
	if (!installHook((LPVOID)address, (LPVOID)detour_pushToActionRequestsQueue, (LPVOID*)&original_pushToActionRequestsQueue, "pushToActionRequestsQueue")) {
		return false;
	}
	return true;
}


bool installFilterPreFinalActionsQueueHook(uintptr_t start, uintptr_t end)
{
	uintptr_t address = start + reverse::offset_filterPreFinalActionsQueue;
	if (!installHook((LPVOID)address, (LPVOID)detour_filterPreFinalActionsQueue, (LPVOID*)&original_filterPreFinalActionsQueue, "filterPreFinalActionsQueue")) {
		return false;
	}
	return true;
}