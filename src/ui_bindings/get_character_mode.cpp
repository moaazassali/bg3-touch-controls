#include "game_bindings.h"
#include "utils.h"

using characterModeHandler_t = int64_t(*)(void* selectedCharacter);
characterModeHandler_t original_characterModeHandler = nullptr;


bool findCharacterModeFunction(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_characterMode = "0F B6 81 10 06 00 00 C3";
	uintptr_t matchAddr = findPattern(pattern_characterMode, start, end);
	if (!matchAddr) {
		printPatternNotFoundError(pattern_characterMode, "CharacterModeHandler");
		return false;
	}
	original_characterModeHandler = (characterModeHandler_t)(matchAddr);
	return true;
}


void* getSelectedCharacter();


int64_t getCharacterModeID()
{
	if (!original_characterModeHandler) {
		return -1;
	}
	void* selectedCharacter = getSelectedCharacter();
	if (!selectedCharacter) {
		return -1;
	}
	return original_characterModeHandler(selectedCharacter);
}