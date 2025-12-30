#include "game_bindings.h"
#include "utils.h"


// Binding Path=DataContext.CurrentPlayer.SelectedCharacter.IsCurrentTurn
// We make a distinction here that this is for 'SelectedCharacter' binding specifically
// because the same property exists under 'CurrentPlayer' as well
// Returns true if the selected character is the one whose turn it currently is in combat or FTB mode
// This also means that if more than one player-controlled characters are after each other in turn order,
// they will all return true when the player switches to any one of them.
using selectedCharacterIsCurrentTurnHandler_t = bool(*)(void* selectedCharacter);
selectedCharacterIsCurrentTurnHandler_t original_selectedCharacterIsCurrentTurnHandler = nullptr;


bool findSelectedCharacterIsCurrentTurnHandlerFunction(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_selectedCharacterIsCurrentTurn = "E8 ?? ?? ?? ?? 84 C0 75 52 49 8B CF";
	uintptr_t matchAddr = findPattern(pattern_selectedCharacterIsCurrentTurn, start, end);
	if (!matchAddr) {
		printPatternNotFoundError(pattern_selectedCharacterIsCurrentTurn, "SelectedCharacterIsCurrentTurnHandler");
		return false;
	}
	// Calculate the target function address from the CALL instruction
	int32_t relativeOffset = *reinterpret_cast<int32_t*>(matchAddr + 1);
	uintptr_t targetAddr = matchAddr + 5 + relativeOffset;
	original_selectedCharacterIsCurrentTurnHandler = (selectedCharacterIsCurrentTurnHandler_t)(targetAddr);
	return true;
}


void* getSelectedCharacter();


bool selectedCharacterIsCurrentTurn()
{
	if (!original_selectedCharacterIsCurrentTurnHandler) {
		return false;
	}
	void* selectedCharacter = getSelectedCharacter();
	if (!selectedCharacter) {
		return false;
	}
	return original_selectedCharacterIsCurrentTurnHandler(selectedCharacter);
}