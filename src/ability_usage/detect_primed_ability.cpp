#include "game_bindings.h"
#include "utils.h"


// This function's a1 is some sort of manager or context, which seems constant throughout the game session.
// The function assigns some ability state struct, which changes often when abilities are used.
// Specifically, it does this: *(a1 + 88) = 0
// In this case it just resets but this field gets assigned somewhere else with the current 'active context'
// When an ability is active, the field is assigned to an 'ability state' struct pointer.
// So, we need to know the that a1 + 88 is an ability state pointer first using it's vtable.
using unk_hasPointerToAbilityState_t = void* (*)(void* a1, void* a2);
unk_hasPointerToAbilityState_t original_unk_hasPointerToAbilityState = nullptr;

static void* unk_a1_manager = nullptr;
// There is only 1 reference to this function, which is the ability state vtable at offset 0x00
// As such this is enough to uniquely identify the active context as an ability state type.
static void* pAbilityStateVtable_function0x00 = nullptr;


static void* detour_unk_hasPointerToAbilityState(void* a1, void* a2)
{
	if (a1 && a2)
	{
		unk_a1_manager = a1;
	}
	return original_unk_hasPointerToAbilityState(a1, a2);
}


bool installUnk_hasPointerToAbilityStateHook(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_unk_hasPointerToAbilityState = "48 89 54 24 10 53 56 57 41 56 48";
	uintptr_t matchAddr = findPattern(pattern_unk_hasPointerToAbilityState, start, end);
	return installHook((void*)matchAddr, &detour_unk_hasPointerToAbilityState, (void**)&original_unk_hasPointerToAbilityState, "unk_hasPointerToAbilityStateHook");
}


bool findAbilityStateVtableFunction0x00(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_abilityStateVtable_function0x00 = "48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 41 56 48 83 EC 20 8B F2 4C 8B F1 48 8D 05 ?? ?? ?? ?? 48 89 01 48 81";
	uintptr_t matchAddr = findPattern(pattern_abilityStateVtable_function0x00, start, end);
	if (!matchAddr) {
		printPatternNotFoundError(pattern_abilityStateVtable_function0x00, "abilityStateVtable_function0x00");
		return false;
	}
	pAbilityStateVtable_function0x00 = (void*)matchAddr;
	return true;
}


bool getAbilityPrimedFlag()
{
	if (!unk_a1_manager || !pAbilityStateVtable_function0x00) {
		return false;
	}
	void* pActiveContext = *(void**)((uintptr_t)unk_a1_manager + 88);
	if (!pActiveContext) {
		return false;
	}
	void** pActiveContextVtable = *(void***)(uintptr_t)pActiveContext;
	if (!pActiveContextVtable) {
		return false;
	}
	void* pActiveContextVtable_function0x00 = pActiveContextVtable[0];
	if (pActiveContextVtable_function0x00 != pAbilityStateVtable_function0x00) {
		return false;
	}
	// Byte at offset 0xE8 is 1 if an ability is primed, 0 otherwise.
	// Although the fact that we have a valid ability state context should be a sufficient
	// indicator that an ability is primed.
	return *(bool*)((uintptr_t)pActiveContext + 0xE8);
}


// [Not used anymore]
// This function gets called whenever an ability is primed.
// In most cases, an ability being primed means it was clicked.
// However, some abilities only get primed after selecting some option like damage type from the hotbar sub-menu (like chromatic orb).
// a1 is the ability state pointer, and it can change after executing the ability, and we end up pointing to invalid data.
// It was initially used but because a1 is not stable, I found addresses in memory that track the ability state instead,
// which led me to the function above that keeps updating the ability state pointer.
using unk_calledOnAbilityPrimed_t = char(*)(void* a1, void* a2, void* a3, char a4, void* a5, void* a6, char a7, char a8, char a9);
unk_calledOnAbilityPrimed_t original_unk_calledOnAbilityPrimed = nullptr;
constexpr const char* pattern_unk_calledOnAbilityPrimed = "40 53 55 56 57 41 55 41 56 41 57 48 81 EC 80 00 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 78";