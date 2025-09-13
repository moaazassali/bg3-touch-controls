#include "game_bindings.h"
#include "Hooks/get_action_requests_buffer_hook.h"


getActionRequestsBuffer_t original_getActionRequestsBuffer = nullptr;


bool installGetActionRequestsBufferHook(uintptr_t start, uintptr_t end)
{
	// we already know the relative offset of the getActionRequestsBuffer method 0x40C79E0, no need for pattern
	// although pattern scanning would be more robust in case of updates
	original_getActionRequestsBuffer = (getActionRequestsBuffer_t)(start + 0x40C79E0);
	return true;
}