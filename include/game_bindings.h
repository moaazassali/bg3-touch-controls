#pragma once
#include <cstdint>
#include <windows.h>

#include "reverse/functions.h"


void bindToGame();
uintptr_t findPattern(const char* pattern, uintptr_t start, uintptr_t end);
bool installHook(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal, const char* hookName);
bool getModuleRange(uintptr_t& start, uintptr_t& end);


bool installUpdateCameraHook(uintptr_t start, uintptr_t end);
bool installCalculatePitchHook(uintptr_t start, uintptr_t end);
bool installUpdateCameraPitchHook(uintptr_t start, uintptr_t end);
bool installProcessInputHook(uintptr_t start, uintptr_t end);


//extern reverse::getActionRequestsBuffer_t original_getActionRequestsBuffer;
//extern reverse::pushToActionRequestsQueue_t original_pushToActionRequestsQueue;
//extern reverse::processActionRequestsQueue_t original_processActionRequestsQueue;