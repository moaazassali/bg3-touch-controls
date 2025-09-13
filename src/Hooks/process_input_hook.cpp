#include "game_bindings.h"
#include "Hooks/process_input_hook.h"
#include "RE/InputManager.h"


processInput_t original_processInput = nullptr;
InputManager* g_inputManager = nullptr;


// This is called every frame to handle input
// a1 is a pretty big "InputManager" struct, which holds a lot of states, queues, etc...
// It process the input events and dispatches a set of predefined actions
// These actions can be mined from the game exe as they are associated with string literals
static void detour_processInput(InputManager* a1, uint64_t a2)
{
	// The pointer to the InputManager singleton is stored at offset 0x6017C58.
	// Our DLL loads before the InputManager singleton is initialized, so we need to set it here
	// where we know that it is valid and not a nullptr anymore.
	if (!g_inputManager) {
		g_inputManager = a1;
	}
	original_processInput(a1, a2);
}


bool installProcessInputHook(uintptr_t start, uintptr_t end)
{
	// we already know the relative offset of the inputmanagerupdat method 0x40C2CC0, no need for pattern
	// although pattern scanning would be more robust in case of updates
	uintptr_t targetAddr = start + 0x40C2CC0;
	return installHook((void*)targetAddr, &detour_processInput, (void**)&original_processInput, "processInput");
}