#include "game_bindings.h"
#include "utils.h"
#include <iostream>


// This is the function registered to handle the 'InTurnBasedMode' UI binding.
using inTurnBasedModeHandler_t = bool(*)(void* selectedCharacter);
inTurnBasedModeHandler_t original_inTurnBasedModeHandler = nullptr;


bool findInTurnBasedModeFunction(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_inTurnBasedMode = "E8 ?? ?? ?? ?? 84 C0 74 5E 48 8B CF";
	uintptr_t matchAddr = findPattern(pattern_inTurnBasedMode, start, end);
	if (!matchAddr) {
		printPatternNotFoundError(pattern_inTurnBasedMode, "InTurnBasedModeHandler");
		return false;
	}
	// Calculate the target function address from the CALL instruction
	int32_t relativeOffset = *reinterpret_cast<int32_t*>(matchAddr + 1);
	uintptr_t targetAddr = matchAddr + 5 + relativeOffset;
	original_inTurnBasedModeHandler = (inTurnBasedModeHandler_t)(targetAddr);
	return true;
}


void* getSelectedCharacter();


bool isInTurnBasedMode()
{
	if (!original_inTurnBasedModeHandler) {
		return false;
	}
	void* selectedCharacter = getSelectedCharacter();
	if (!selectedCharacter) {
		return false;
	}
	return original_inTurnBasedModeHandler(selectedCharacter);
}