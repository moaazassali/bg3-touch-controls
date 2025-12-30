#include "game_bindings.h"
#include "utils.h"


// This is needed for the 'PlayerCharacterProperties' UI binding, which has
// sub-properties like 'IsCasting', 'CurrentSpellTask', etc...
using playerCharacterPropertiesHandler_t = void* (*)(void* selectedCharacter);
playerCharacterPropertiesHandler_t original_playerCharacterPropertiesHandler = nullptr;


bool findPlayerCharacterPropertiesFunction(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_playerCharacterProperties = "E8 ?? ?? ?? ?? 48 8B D8 48 85 C0 74 3A 80 3F 00";
	uintptr_t matchAddr = findPattern(pattern_playerCharacterProperties, start, end);
	if (!matchAddr) {
		printPatternNotFoundError(pattern_playerCharacterProperties, "PlayerCharacterPropertiesHandler");
		return false;
	}
	// Calculate the target function address from the CALL instruction
	int32_t relativeOffset = *reinterpret_cast<int32_t*>(matchAddr + 1);
	uintptr_t targetAddr = matchAddr + 5 + relativeOffset;
	original_playerCharacterPropertiesHandler = (playerCharacterPropertiesHandler_t)(targetAddr);
	return true;
}


void* getSelectedCharacter();


void* getPlayerCharacterProperties()
{
	if (!original_playerCharacterPropertiesHandler) {
		return nullptr;
	}
	void* selectedCharacter = getSelectedCharacter();
	if (!selectedCharacter) {
		return nullptr;
	}
	return original_playerCharacterPropertiesHandler(selectedCharacter);
}