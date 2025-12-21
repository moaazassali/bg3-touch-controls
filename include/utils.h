#pragma once
#include <functional>
#include <chrono>
#include <wtypes.h>


void scheduleTask(std::function<void()> task, std::chrono::milliseconds delay);

SIZE getWindowSizePx(HWND hwnd);

void printModuleInstallHeader(const char* moduleName);

void printModuleInstallResult(bool success);

void printPatternNotFoundError(const char* pattern, const char* patternName);

void printPatternFoundMessage(uintptr_t address, const char* patternName);