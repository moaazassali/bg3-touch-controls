#include "ui_bindings.h"
#include "utils.h"


bool installSyncGameStateToUIHook(uintptr_t start, uintptr_t end);
bool findGetDataContextFunction(uintptr_t start, uintptr_t end);
bool findGetPlayerCountFunction(uintptr_t start, uintptr_t end);
bool findGetPlayerListHeadFunction(uintptr_t start, uintptr_t end);
bool findGetCurrentPlayerFunction(uintptr_t start, uintptr_t end);
bool findGetSelectedCharacterFunction(uintptr_t start, uintptr_t end);
bool findInTurnBasedModeFunction(uintptr_t start, uintptr_t end);
bool findCharacterModeFunction(uintptr_t start, uintptr_t end);
bool findPlayerCharacterPropertiesFunction(uintptr_t start, uintptr_t end);
bool findIsCastingFunction(uintptr_t start, uintptr_t end);
bool findMovingHandlerFunction(uintptr_t start, uintptr_t end);
bool findCurrentSpellTaskHandlerFunction(uintptr_t start, uintptr_t end);
bool findSelectedCharacterIsCurrentTurnHandlerFunction(uintptr_t start, uintptr_t end);


bool installUIBindingsModule(uintptr_t start, uintptr_t end) {
	printModuleInstallHeader("UI Bindings");
	bool success = true;
	success &= installSyncGameStateToUIHook(start, end);
	success &= findGetDataContextFunction(start, end);
	success &= findGetPlayerCountFunction(start, end);
	success &= findGetPlayerListHeadFunction(start, end);
	success &= findGetCurrentPlayerFunction(start, end);
	success &= findGetSelectedCharacterFunction(start, end);
	success &= findInTurnBasedModeFunction(start, end);
	success &= findCharacterModeFunction(start, end);
	success &= findPlayerCharacterPropertiesFunction(start, end);
	success &= findIsCastingFunction(start, end);
	success &= findMovingHandlerFunction(start, end);
	success &= findCurrentSpellTaskHandlerFunction(start, end);
	success &= findSelectedCharacterIsCurrentTurnHandlerFunction(start, end);
	printModuleInstallResult(success);
	return success;
}


bool isInTurnBasedMode();
int64_t getCharacterModeID();
bool isCasting();
bool isMoving();
void* getCurrentSpellTask();
bool selectedCharacterIsCurrentTurn();


bool isSelectedCharacterInTurnBasedMode() {
	return isInTurnBasedMode();
}


CharacterMode getSelectedCharacterMode() {
	int64_t modeID = getCharacterModeID();
	switch (modeID) {
	case 1:
		return CharacterMode::Idle;
	case 2:
		return CharacterMode::InCombat;
	case 4:
		return CharacterMode::InFTB;
	case 8:
		return CharacterMode::InDialogue;
	case 16:
		return CharacterMode::InTrade;
	default:
		return CharacterMode::Invalid;
	}
}


bool isSelectedCharacterCasting() {
	return isCasting();
}


bool isSelectedCharacterMoving() {
	return isMoving();
}


bool selectedCharacterHasAbilityPrimed() {
	void* spellTask = getCurrentSpellTask();
	return spellTask != nullptr;
}


bool isSelectedCharacterTurn() {
	return selectedCharacterIsCurrentTurn();
}