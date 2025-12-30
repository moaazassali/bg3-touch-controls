#include "ui_bindings.h"
#include "utils.h"


bool installUIBindingsModule(uintptr_t start, uintptr_t end) {
	printModuleInstallHeader("UI Bindings");
	bool success = true;
	printModuleInstallResult(success);
	return success;
}