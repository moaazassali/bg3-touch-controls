#pragma once
#include <cstdint>
#include <windows.h>
#include <vector>

#include "reverse/functions.h"

// Assumes that MH_Initialize has already been called
void bindToGame();
uintptr_t findPattern(const char* pattern, uintptr_t start, uintptr_t end);
bool asmPatchAtOffset(uintptr_t address, std::size_t offset, const std::vector<uint8_t>& bytes, const char* patchName);
bool installHook(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal, const char* hookName);


bool installCalculatePitchHook(uintptr_t start, uintptr_t end);
bool installFilterPreFinalActionsQueueHook(uintptr_t start, uintptr_t end);
bool installMoveCharacterHook(uintptr_t start, uintptr_t end);

bool installMinimumHotbarRowsPatch(uintptr_t start, uintptr_t end);
bool installRunMoveCharacterwithoutControllerPatch(uintptr_t start, uintptr_t end);

bool installRegisterHotBarUIBindingsHook(uintptr_t start, uintptr_t end);
bool installIncreaseHotBarUIBindingHandlersStructSizePatch(uintptr_t start, uintptr_t end);
bool installRegisterHotBarUIBindingHandlersHook(uintptr_t start, uintptr_t end);


extern reverse::processActionRequestsQueue_t original_processActionRequestsQueue;
extern reverse::filterPreFinalActionsQueue_t original_filterPreFinalActionsQueue;

extern bool g_gameWorldLoaded;