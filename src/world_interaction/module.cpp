#include "world_interaction.h"
#include "utils.h"


bool installWorldInteractionTargetResolverHook(uintptr_t start, uintptr_t end);


bool installWorldInteractionModule(uintptr_t start, uintptr_t end) {
	printModuleInstallHeader("World Interaction");
	bool success = true;
	success &= installWorldInteractionTargetResolverHook(start, end);
	printModuleInstallResult(success);
	return success;
}


void bypassWorldInteractionTargetResolver();
void useOriginalWorldInteractionTargetResolver();


void disableWorldHoverEffects() {
	bypassWorldInteractionTargetResolver();
}


void enableWorldHoverEffects() {
	useOriginalWorldInteractionTargetResolver();
}