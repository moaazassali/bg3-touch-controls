#include "Hooks/hooks_utils.h"
#include "Hooks/get_action_requests_buffer_hook.h"


getActionRequestsBuffer_t original_getActionRequestsBuffer = nullptr;


// This function retrieves the raw action buffer that holds all actions that are mapped to the
// input provided by the player.
// So the actions in this buffer are all unfiltered and unprocessed.
// A later stage will process these actions and filter them based on the current game state.
// i.e. a mouse scroll up event maps to ui scroll and camera zoom, but if the ui is open
// only the ui scroll action should be used, so the camera zoom action is filtered out.
// a1 is the address of what seems to be a list of action buffers, one for game "context"/player.
// it is at offset 0x40 of the InputManager singleton instance.
// a2 is the current game "context" which is used for local co-op.
// 0 = player 1 (i.e. when playing solo or as the first player in local co-op)
static void detour_getActionRequestsBuffer(void* a1, uint8_t a2)
{
	original_getActionRequestsBuffer(a1, a2);
}


bool installGetActionRequestsBufferHook(uintptr_t start, uintptr_t end)
{
	// we already know the relative offset of the getActionRequestsBuffer method 40C 79E0, no need for pattern
	// although pattern scanning would be more robust in case of updates
	uintptr_t targetAddr = start + 0x40C79E0;
	return installHook((void*)targetAddr, &detour_getActionRequestsBuffer, (void**)&original_getActionRequestsBuffer, "getActionRequestsBuffer");
}