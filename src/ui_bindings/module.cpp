#include "ui_bindings.h"
#include "utils.h"


bool installSyncGameStateToUIHook(uintptr_t start, uintptr_t end);
bool findGetDataContextFunction(uintptr_t start, uintptr_t end);


bool installUIBindingsModule(uintptr_t start, uintptr_t end) {
	printModuleInstallHeader("UI Bindings");
	bool success = true;
	success &= installSyncGameStateToUIHook(start, end);
	success &= findGetDataContextFunction(start, end);
	printModuleInstallResult(success);
	return success;
}