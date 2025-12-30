#include "game_bindings.h"
#include "utils.h"

using getSelectedCharacter_t = void* (*)(void* currentPlayer);
getSelectedCharacter_t original_getSelectedCharacter = nullptr;


bool findGetSelectedCharacterFunction(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_getSelectedCharacter = "E8 ?? ?? ?? ?? 48 8B F0 48 89 45 28";
	uintptr_t matchAddr = findPattern(pattern_getSelectedCharacter, start, end);
	if (!matchAddr) {
		printPatternNotFoundError(pattern_getSelectedCharacter, "getSelectedCharacter");
		return false;
	}
	// Calculate the target function address from the CALL instruction
	int32_t relativeOffset = *reinterpret_cast<int32_t*>(matchAddr + 1);
	uintptr_t targetAddr = matchAddr + 5 + relativeOffset;
	original_getSelectedCharacter = (getSelectedCharacter_t)(targetAddr);
	return true;
}


void* getDataContext();
void* getCurrentPlayer();


void* getSelectedCharacter()
{
	if (!original_getSelectedCharacter) {
		return nullptr;
	}

	void* dataContext = getDataContext();
	if (!dataContext) {
		return nullptr;
	}

	void* currentPlayer = getCurrentPlayer();
	if (!currentPlayer) {
		return nullptr;
	}

	return original_getSelectedCharacter(currentPlayer);
}