#include "game_camera.h"
#include "utils.h"


bool installGameCameraModule(uintptr_t start, uintptr_t end) {
	printModuleInstallHeader("Game Camera");
	bool success = true;
	printModuleInstallResult(success);
	return success;
}