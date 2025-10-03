#include "game_bindings.h"
#include "reverse/functions.h"


reverse::unk_consumesFilteredActionsQueue_t original_unk_consumesFilteredActionsQueue = nullptr;


namespace reverse {
	bool* g_isCursorOverUI = nullptr;
}


static void detour_unkConsumesFilteredActionsQueue(void* a1, double* a2)
{
	if (!reverse::g_isCursorOverUI) {
		// The boolean that indicates whether the cursor is over UI is at offset 0x240 of a1
		reverse::g_isCursorOverUI = reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(a1) + 0x240);
	}
	original_unk_consumesFilteredActionsQueue(a1, a2);
}


bool installUnkConsumesFilteredActionsQueueHook(uintptr_t start, uintptr_t end)
{
	uintptr_t targetAddr = start + reverse::offset_unk_consumesFilteredActionsQueue;
	return installHook((void*)targetAddr, &detour_unkConsumesFilteredActionsQueue, (void**)&original_unk_consumesFilteredActionsQueue, "unkConsumesFilteredActionsQueueHook");
}