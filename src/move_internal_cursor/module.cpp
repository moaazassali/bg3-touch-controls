#include "utils.h"
#include "game_bindings.h"


bool installRawInputHandlerHook(uintptr_t start, uintptr_t end);
bool getInternalCursorCoordinatesAddress(uintptr_t start, uintptr_t end);
bool installNoesisHitTestHook(uintptr_t start, uintptr_t end);
bool installUpdateCursorOverUIFlagHook(uintptr_t start, uintptr_t end);


bool installMoveInternalCursorModule(uintptr_t start, uintptr_t end) {
	printModuleInstallHeader("Move Internal Cursor");
	bool success = true;
	success &= installRawInputHandlerHook(start, end);
	success &= getInternalCursorCoordinatesAddress(start, end);
	success &= installNoesisHitTestHook(start, end);
	success &= installUpdateCursorOverUIFlagHook(start, end);
	printModuleInstallResult(success);
	return success;
}


void updateInternalCursorCoordinates(float x, float y);
bool updateCursorOverUIFlag(float x, float y);
bool getIsCursorOverUIFlag();


// x and y are window coordinates from top-left corner in pixels, not screen coordinates
bool moveInternalCursor(float x, float y) {
	updateInternalCursorCoordinates(x, y);
	return updateCursorOverUIFlag(x, y);
}


bool isCursorOverUI() {
	return getIsCursorOverUIFlag();
}