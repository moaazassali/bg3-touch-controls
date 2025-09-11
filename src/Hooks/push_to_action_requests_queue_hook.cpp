#include "Hooks/push_to_action_requests_queue_hook.h"

pushToActionRequestsQueue_t original_pushToActionRequestsQueue = nullptr;

bool installPushToActionRequestsQueueHook(uintptr_t start, uintptr_t end)
{
	// we already know the relative offset of the pushToActionRequestsQueue method 0x121C460, no need for pattern
	// although pattern scanning would be more robust in case of updates
	// simply assign the original function to address no need for hooking
	original_pushToActionRequestsQueue = (pushToActionRequestsQueue_t)(start + 0x121C460);
	return true;
}