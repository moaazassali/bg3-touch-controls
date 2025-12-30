#include "game_bindings.h"
#include "utils.h"


// I found this function when trying to see what calls the UI function associated with
// the 'InTurnBasedMode' binding. From that line, I traced back how to get the selected character
// all the way up to the main 'ui manager', which I assume is a1 here.
using syncGameStateToUI_t = int64_t (*)(void* a1, void* a2, float deltaTime);
syncGameStateToUI_t original_syncGameStateToUI = nullptr;

static void* uiManager = nullptr;


static int64_t detour_syncGameStateToUI(void* a1, void* a2, float deltaTime)
{
	if (a1) {
		uiManager = a1;
	}
	return original_syncGameStateToUI(a1, a2, deltaTime);
}


bool installSyncGameStateToUIHook(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_syncGameStateToUI = "48 8B C4 48 89 58 20 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 38 F9";
	uintptr_t matchAddr = findPattern(pattern_syncGameStateToUI, start, end);
	return installHook((void*)matchAddr, &detour_syncGameStateToUI, (void**)&original_syncGameStateToUI, "syncGameStateToUI");
}


// This gives us the 'DataContext' from UI Bindings like these:
// Binding CurrentPlayer.SelectedCharacter.InTurnBasedMode
using getDataContext_t = void* (*)(void* a1);
getDataContext_t original_getDataContext = nullptr;


bool findGetDataContextFunction(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_getDataContext = "40 53 48 83 EC 20 48 8B 05 ?? ?? ?? ?? 48 8B 59 08 48 85 C0 75 21 4C 8D 05 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8B D3 48 8B C8 48";
	uintptr_t matchAddr = findPattern(pattern_getDataContext, start, end);
	if (!matchAddr) {
		printPatternNotFoundError(pattern_getDataContext, "getDataContext");
		return false;
	}
	original_getDataContext = (getDataContext_t)(matchAddr);
	return true;
}


void* getDataContext()
{
	if (!uiManager || !original_getDataContext) {
		return nullptr;
	}
	return original_getDataContext(uiManager);
}