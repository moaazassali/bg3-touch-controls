#pragma once
#include <cstdint>

#include "reverse/action_request.h"


bool installActionRequestsModule(uintptr_t start, uintptr_t end);

void triggerActionRequestQueueProcessing();

using pushToActionRequestQueue_t = reverse::ActionRequest* (*)(void* a1, reverse::ActionRequest* a2);
extern pushToActionRequestQueue_t original_pushToActionRequestQueue;