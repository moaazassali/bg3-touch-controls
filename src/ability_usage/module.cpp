#include "ability_usage.h"
#include "utils.h"


bool installAbilityUsageModule(uintptr_t start, uintptr_t end) {
	printModuleInstallHeader("Ability Usage");
	bool success = true;
	printModuleInstallResult(success);
	return success;
}