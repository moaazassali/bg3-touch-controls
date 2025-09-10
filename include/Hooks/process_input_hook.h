#pragma once
#include <cstdint>

#include "RE/InputManager.h"


using processInput_t = void(*)(InputManager* a1, uint64_t a2);

bool installProcessInputHook(uintptr_t start, uintptr_t end);