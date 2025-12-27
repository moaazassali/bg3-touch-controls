#include "move_internal_cursor.h"
#include "utils.h"
#include "world_interaction.h"


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
void registerDeferredTask(int framesDelay, std::function<void()> func);


// x and y are window coordinates from top-left corner in pixels, not screen coordinates
bool moveInternalCursor(float x, float y) {
	updateInternalCursorCoordinates(x, y);
	updateWorldInteractions();
	return updateCursorOverUIFlag(x, y);
}


bool isCursorOverUI() {
	return getIsCursorOverUIFlag();
}


void callAfterGameProcessesCursorUpdate(std::function<void()> func) {
	// 2 frames is the minimum delay needed to guarantee the game processed the cursor update
	// If the hooked function is not running right now, then our cursor update will be processed
	// in the next frame, so we need to wait one more frame after that.
	// In the rare case the hooked function is running right now, and our cursor update happens
	// before it decides to trigger cursor update side effects, then 1 frame will be enough.
	// So to cover all cases, we use 2 frames delay.
	constexpr int framesDelay = 2;
	registerDeferredTask(framesDelay, func);
}