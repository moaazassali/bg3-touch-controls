#pragma once
#include <cstdint>


bool installWorldInteractionModule(uintptr_t start, uintptr_t end);

void freezeProcessingWorldEntityInteractions();

void resumeProcessingWorldEntityInteractions();

void clearActiveWorldEntityInteractions();

enum class WorldEntityType {
	None = 0,
	NonInteractable = 1,
	Item = 2,
	NPC = 3,
	Corpse = 4,
};


struct WorldEntity {
	WorldEntityType type;
	uint64_t id;
};


WorldEntity getCurrentWorldTargetEntity();