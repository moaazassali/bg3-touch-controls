#pragma once
#include <cstdint>


using processInput_t = void(*)(void* a1, uint64_t a2);

bool installProcessInputHook(uintptr_t start, uintptr_t end);