#include "utils.h"
#include "game_bindings.h"


bool installRawInputHandlerHook(uintptr_t start, uintptr_t end);
bool getInternalCursorCoordinatesAddress(uintptr_t start, uintptr_t end);


bool installMoveInternalCursorModule(uintptr_t start, uintptr_t end) {
	printModuleInstallHeader("Move Internal Cursor");
	bool success = true;
	success &= installRawInputHandlerHook(start, end);
	success &= getInternalCursorCoordinatesAddress(start, end);
	printModuleInstallResult(success);
	return success;
}

// TODO: Add public api to move the internal cursor by specified amounts