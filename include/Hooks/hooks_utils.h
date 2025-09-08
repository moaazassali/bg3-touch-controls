#pragma once
#include <cstdint>
#include <windows.h>


uintptr_t findPattern(const char* pattern, uintptr_t start, uintptr_t end);
bool getModuleRange(uintptr_t& start, uintptr_t& end);
bool installHook(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal, const char* hookName);