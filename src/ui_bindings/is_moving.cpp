#include "game_bindings.h"
#include "utils.h"
#include <iostream>


// ALways false regardless of actual movement state somehow...
using movingHandler_t = bool(*)(void* playerCharacterProperties);
movingHandler_t original_movingHandler = nullptr;


bool findMovingHandlerFunction(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_movingHandler = "E8 ?? ?? ?? ?? 3C 01 0F 94 44 24 48";
	uintptr_t matchAddr = findPattern(pattern_movingHandler, start, end);
	if (!matchAddr) {
		printPatternNotFoundError(pattern_movingHandler, "MovingHandler");
		return false;
	}
	// Calculate the target function address from the CALL instruction
	int32_t relativeOffset = *reinterpret_cast<int32_t*>(matchAddr + 1);
	uintptr_t targetAddr = matchAddr + 5 + relativeOffset;
	original_movingHandler = (movingHandler_t)(targetAddr);
	return true;
}


void* getPlayerCharacterProperties();


bool isMoving()
{
	if (!original_movingHandler) {
		return false;
	}
	void* playerCharacterProperties = getPlayerCharacterProperties();
	if (!playerCharacterProperties) {
		return false;
	}
	return original_movingHandler(playerCharacterProperties);
}