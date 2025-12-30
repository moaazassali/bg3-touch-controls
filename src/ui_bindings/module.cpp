#include "ui_bindings.h"
#include "utils.h"


bool installSyncGameStateToUIHook(uintptr_t start, uintptr_t end);
bool findGetDataContextFunction(uintptr_t start, uintptr_t end);
bool findGetPlayerCountFunction(uintptr_t start, uintptr_t end);
bool findGetPlayerListHeadFunction(uintptr_t start, uintptr_t end);
bool findGetCurrentPlayerFunction(uintptr_t start, uintptr_t end);
bool findGetSelectedCharacterFunction(uintptr_t start, uintptr_t end);
bool findInTurnBasedModeFunction(uintptr_t start, uintptr_t end);


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
	printModuleInstallResult(success);
	return success;
}


bool isInTurnBasedMode();


bool isSelectedCharacterInTurnBasedMode() {
	return isInTurnBasedMode();
}