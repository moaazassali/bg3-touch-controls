#pragma once
#include <cstdint>


bool installWorldInteractionModule(uintptr_t start, uintptr_t end);

void freezeProcessingWorldEntityInteractions();

void resumeProcessingWorldEntityInteractions();

void clearActiveWorldEntityInteractions();