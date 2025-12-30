#include "game_camera.h"
#include "utils.h"


bool installUpdateCameraHook(uintptr_t start, uintptr_t end);


bool installGameCameraModule(uintptr_t start, uintptr_t end) {
	printModuleInstallHeader("Game Camera");
	bool success = true;
	success &= installUpdateCameraHook(start, end);
	printModuleInstallResult(success);
	return success;
}