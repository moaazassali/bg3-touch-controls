#include <array>

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
void resetWorldInteractionTargetResolver_a1();
std::array<uint64_t, 5> getCurrentWorldTargetEntityHandles();


void freezeProcessingWorldEntityInteractions() {
	bypassWorldInteractionTargetResolver();
}


void resumeProcessingWorldEntityInteractions() {
	useOriginalWorldInteractionTargetResolver();
}


void clearActiveWorldEntityInteractions() {
	resetWorldInteractionTargetResolver_a1();
}


WorldEntity getCurrentWorldTargetEntity() {
	constexpr uint64_t NULL_HANDLE = 0xFFC0000000000000;
	auto handles = getCurrentWorldTargetEntityHandles();

	if (handles[0] != NULL_HANDLE) {
		return { WorldEntityType::NPC, handles[0] };
	}
	else if (handles[1] != NULL_HANDLE) {
		return { WorldEntityType::Corpse, handles[1] };
	}
	else if (handles[2] != NULL_HANDLE) {
		return { WorldEntityType::Item, handles[2] };
	}
	// Everything else is NULL, so this must be a non-interactable object
	else if (handles[4] != NULL_HANDLE) {
		return { WorldEntityType::NonInteractable, handles[4] };
	}
	else {
		return { WorldEntityType::None, 0 };
	}
}