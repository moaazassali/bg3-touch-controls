#pragma once
#include <cstdint>


using getActionRequestsBuffer_t = void(*)(void* a1, uint8_t a2);

bool installGetActionRequestsBufferHook(uintptr_t start, uintptr_t end);