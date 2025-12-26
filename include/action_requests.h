#pragma once
#include <cstdint>

#include "reverse/action_request.h"


bool installActionRequestsModule(uintptr_t start, uintptr_t end, bool enableDebug);

void triggerActionRequestQueueProcessing();

using pushToActionRequestQueue_t = reverse::ActionRequest* (*)(void* a1, reverse::ActionRequest* a2);
extern pushToActionRequestQueue_t original_pushToActionRequestQueue;

using getActionRequestsQueuesList_t = void* (*)(void* a1, uint8_t* a2);
extern getActionRequestsQueuesList_t original_getActionRequestsQueuesList;