#pragma once
#include <cstdint>


bool installActionRequestsModule(uintptr_t start, uintptr_t end);

void triggerActionRequestQueueProcessing();