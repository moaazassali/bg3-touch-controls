#include "game_bindings.h"
#include <iostream>


// This function gets called whenever an ability is primed.
// In most cases, an ability being primed means it was clicked.
// However, some abilities only get primed after selecting some option like damage type from the hotbar sub-menu (like chromatic orb).
using unk_hasPointerToAbilityState_t = char(*)(void* a1, void* a2, void* a3, char a4, void* a5, void* a6, char a7, char a8, char a9);
unk_hasPointerToAbilityState_t original_unk_hasPointerToAbilityState = nullptr;

static void* pAbilityState = nullptr;


static char detour_unk_hasPointerToAbilityState(void* a1, void* a2, void* a3, char a4, void* a5, void* a6, char a7, char a8, char a9)
{
	if (a1)
	{
		// [Not fully verified] the ability state address seems to change sometimes,
		// so we can't rely on storing it just once.
		pAbilityState = a1;
	}
	// print a1 addr in hex
	std::cout << "unk_hasPointerToAbilityState called with a1: " << std::hex << (uintptr_t)a1 << std::dec << std::endl;
	return original_unk_hasPointerToAbilityState(a1, a2, a3, a4, a5, a6, a7, a8, a9);
}


bool installUnk_hasPointerToAbilityStateHook(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_unk_hasPointerToAbilityState = "40 53 55 56 57 41 55 41 56 41 57 48 81 EC 80 00 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 78";
	uintptr_t matchAddr = findPattern(pattern_unk_hasPointerToAbilityState, start, end);
	return installHook((void*)matchAddr, &detour_unk_hasPointerToAbilityState, (void**)&original_unk_hasPointerToAbilityState, "unk_hasPointerToAbilityStateHook");
}


bool getAbilityPrimedFlag()
{
	if (!pAbilityState) {
		// Correct to assume no ability is primed if we don't have the state pointer yet.
		// Our hooked function must be called whenever an ability is used.
		// So, if an ability is primed, the pointer must exist.
		return false;
	}
	// Byte at offset 0xE8 is 1 if an ability is primed, 0 otherwise.
	return *(bool*)((uintptr_t)pAbilityState + 0xE8);
}