#include "Hooks/process_action_requests_queue_hook.h"


processActionRequestsQueue_t original_processActionRequestsQueue = nullptr;


bool installProcessActionRequestsQueueHook(uintptr_t start, uintptr_t end)
{
	// pattern scanning would be more robust in case of updates
	original_processActionRequestsQueue = (processActionRequestsQueue_t)(start + 0x40C41C0);
	return true;
}
