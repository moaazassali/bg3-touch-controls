#include "game_bindings.h"
#include "reverse/input_manager.h"


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