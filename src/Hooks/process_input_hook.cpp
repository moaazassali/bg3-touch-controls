#include "Hooks/hooks_utils.h"
#include "Hooks/process_input_hook.h"


processInput_t original_processInput = nullptr;


static void detour_processInput(void* a1, uint64_t a2)
{
	original_processInput(a1, a2);
}


bool installProcessInputHook(uintptr_t start, uintptr_t end)
{
	// we already know the relative offset of the inputmanagerupdat method 0x40C2CC0, no need for pattern
	// although pattern scanning would be more robust in case of updates
	uintptr_t targetAddr = start + 0x40C2CC0;
	return installHook((void*)targetAddr, &detour_processInput, (void**)&original_processInput, "processInput");
}