#include "game_bindings.h"
#include "utils.h"
#include <iostream>


// returns 0 when no current spell task is active
// returns 0 when the ability was confirmed and is in the process of being cast
// returns 0 when an ability is selected but not yet ready/primed for execution
// like when an option like damage type needs to be selected first
// returns the spell task pointer when an ability is primed and ready for execution
// i.e. next click will execute the ability
using currentSpellTaskHandler_t = void* (*)(void* playerCharacterProperties);
currentSpellTaskHandler_t original_currentSpellTaskHandler = nullptr;


bool findCurrentSpellTaskHandlerFunction(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_currentSpellTask = "48 8B 81 30 06 00 00";
	uintptr_t matchAddr = findPattern(pattern_currentSpellTask, start, end);
	if (!matchAddr) {
		printPatternNotFoundError(pattern_currentSpellTask, "CurrentSpellTaskHandler");
		return false;
	}
	original_currentSpellTaskHandler = (currentSpellTaskHandler_t)(matchAddr);
	return true;
}


void* getPlayerCharacterProperties();


void* getCurrentSpellTask()
{
	if (!original_currentSpellTaskHandler) {
		return nullptr;
	}
	void* playerCharacterProperties = getPlayerCharacterProperties();
	if (!playerCharacterProperties) {
		return nullptr;
	}
	auto spell = original_currentSpellTaskHandler(playerCharacterProperties);
	return spell;
}