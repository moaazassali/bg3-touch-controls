#include "game_bindings.h"
#include "utils.h"


// Binding Path=DataContext.CurrentPlayer.SelectedCharacter.PlayerCharacterProperties.IsCasting
// It is true when the selected character has an ability primed and ready for execution pending a click.
// It is also true when the character is in the middle of casting an ability (when the animation is playing).
// It is also true for 'castable' items like throwable items, scrolls, etc...
using isCastingHandler_t = bool(*)(void* playerCharacterProperties);
isCastingHandler_t original_isCastingHandler = nullptr;


bool findIsCastingFunction(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_isCasting = "0F B6 81 50 06";
	uintptr_t matchAddr = findPattern(pattern_isCasting, start, end);
	if (!matchAddr) {
		printPatternNotFoundError(pattern_isCasting, "IsCastingHandler");
		return false;
	}
	original_isCastingHandler = (isCastingHandler_t)(matchAddr);
	return true;
}


void* getPlayerCharacterProperties();


bool isCasting()
{
	if (!original_isCastingHandler) {
		return false;
	}
	void* playerCharacterProperties = getPlayerCharacterProperties();
	if (!playerCharacterProperties) {
		return false;
	}
	return original_isCastingHandler(playerCharacterProperties);
}