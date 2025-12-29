#include "ability_usage.h"
#include "utils.h"


bool installUnk_hasPointerToAbilityStateHook(uintptr_t start, uintptr_t end);


bool installAbilityUsageModule(uintptr_t start, uintptr_t end) {
	printModuleInstallHeader("Ability Usage");
	bool success = true;
	success &= installUnk_hasPointerToAbilityStateHook(start, end);
	printModuleInstallResult(success);
	return success;
}


bool getAbilityPrimedFlag();


bool isAnAbilityPrimed() {
	return getAbilityPrimedFlag();
}