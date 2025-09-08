#pragma once
#include <cstdint>


using getRawActionBuffer_t = void(*)(void* a1, uint8_t a2);

bool installGetRawActionBufferHook(uintptr_t start, uintptr_t end);