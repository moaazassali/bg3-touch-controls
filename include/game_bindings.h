#pragma once
#include <cstdint>
#include <windows.h>
#include <vector>

#include "reverse/functions.h"
#include "reverse/input_manager.h"

// Assumes that MH_Initialize has already been called
void bindToGame();
uintptr_t findPattern(const char* pattern, uintptr_t start, uintptr_t end);
bool asmPatchAtOffset(uintptr_t address, std::size_t offset, const std::vector<uint8_t>& bytes, const char* patchName);
bool installHook(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal, const char* hookName);


bool installUpdateCameraHook(uintptr_t start, uintptr_t end);
bool installCalculatePitchHook(uintptr_t start, uintptr_t end);
bool installUpdateCameraPitchHook(uintptr_t start, uintptr_t end);
bool installProcessInputHook(uintptr_t start, uintptr_t end);
bool installUnkConsumesFilteredActionsQueueHook(uintptr_t start, uintptr_t end);
bool installPushToActionRequestsQueueHook(uintptr_t start, uintptr_t end);
bool installFilterPreFinalActionsQueueHook(uintptr_t start, uintptr_t end);
bool installMoveCharacterHook(uintptr_t start, uintptr_t end);

bool installMinimumHotbarRowsPatch(uintptr_t start, uintptr_t end);

bool installRegisterHotBarUIBindingsHook(uintptr_t start, uintptr_t end);
bool installIncreaseHotBarUIBindingHandlersStructSizePatch(uintptr_t start, uintptr_t end);
bool installRegisterHotBarUIBindingHandlersHook(uintptr_t start, uintptr_t end);

bool installDX11PresentHook();


extern reverse::getActionRequestsBuffer_t original_getActionRequestsBuffer;
extern reverse::pushToActionRequestsQueue_t original_pushToActionRequestsQueue;
extern reverse::processActionRequestsQueue_t original_processActionRequestsQueue;
extern reverse::filterPreFinalActionsQueue_t original_filterPreFinalActionsQueue;


namespace reverse {
	extern InputManager* g_inputManager;
	extern bool* g_isCursorOverUI;
}