#pragma once
#include <cstdint>

#include "RE/ActionRequest.h"


// This takes a queue from the action requests buffer retrieved via getActionRequestsBuffer
// and pushes a new action request to it.
using pushToActionRequestsQueue_t = ActionRequest* (*)(void* a1, ActionRequest* a2);

extern pushToActionRequestsQueue_t original_pushToActionRequestsQueue;

bool installPushToActionRequestsQueueHook(uintptr_t start, uintptr_t end);