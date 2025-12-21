#include <iostream>

#include "utils.h"
#include "game_bindings.h"


using rawInputHandler_t = void(*)(void* a1, void* a2);
rawInputHandler_t original_updateInternalCursorState = nullptr;

static void* pInternalCursorCoordinates = nullptr;
static void* pWindowInstance = nullptr;
using scaleCoordinatesFunc_t = float* (*)(void* pWindowInstance, float* pOut, float* pIn);
static scaleCoordinatesFunc_t pScaleCoordinatesFunc = nullptr;


static void detour_rawInputHandler(void* a1, void* a2)
{
	// This function handles the raw mouse motion SDL events and updates the internal
	// cursor state coordinates accordingly.
	// It calls a virtual function on the window instance to scale the raw input coordinates
	// in case the window is resized or has a different DPI scaling.
	// The scaled coordinates are then written to the internal cursor position structure.
	// 
	// v69 = *(a1 + 16);
	// v70 = *v69;
	// v113[0] = SHIDWORD(v131);
	// v113[1] = v132;
	// *g_pCursorPosition = *(*(v70 + 264))(v69, v127, v113);
	if (!a1) {
		return original_updateInternalCursorState(a1, a2);
	}

	pWindowInstance = *(void**)((uintptr_t)a1 + 16);
	void** vtable = *(void***)pWindowInstance;
	pScaleCoordinatesFunc = (scaleCoordinatesFunc_t)vtable[33]; // 264 / 8 = 33

	return original_updateInternalCursorState(a1, a2);
}


bool installRawInputHandlerHook(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_rawInputHandler = "48 89 5C 24 18 48 89 7C 24 20 55 41 54 41 55 41 56 41 57 48 8D AC 24 C0";
	uintptr_t matchAddr = findPattern(pattern_rawInputHandler, start, end);
	return installHook((void*)matchAddr, &detour_rawInputHandler, (void**)&original_updateInternalCursorState, "updateInternalCursorStateHook");
}


bool getInternalCursorCoordinatesAddress(uintptr_t start, uintptr_t end)
{
	// .text:00007FF793BE7593    mov    rax, cs:g_pCursorPosition
	// .text:00007FF793BE759A    movsd  qword ptr[rax], xmm0
	constexpr const char* pattern = "48 8B 05 ?? ?? ?? ?? F2 0F 11 00";
	uintptr_t patternAddr = findPattern(pattern, start, end);

	if (!patternAddr) {
		printPatternNotFoundError(pattern, "getInternalCursorCoordinatesAddress");
		return false;
	}

	int32_t relativeOffset = *reinterpret_cast<int32_t*>(patternAddr + 3);
	pInternalCursorCoordinates = reinterpret_cast<void*>(patternAddr + 7 + relativeOffset);
	printPatternFoundMessage((uintptr_t)pInternalCursorCoordinates, "getInternalCursorCoordinatesAddress");
	return true;
}


// The x and y are the coordinates from SDL mouse motion events relative to the
// window, not the screen.
void updateInternalCursorCoordinates(float x, float y) {
	if (!pInternalCursorCoordinates || !pWindowInstance || !pScaleCoordinatesFunc) {
		return;
	}

	float inputCoords[2] = { x, y };
	float scaledCoords[2] = { 0.0f, 0.0f };
	pScaleCoordinatesFunc(pWindowInstance, scaledCoords, inputCoords);
	float* pCursorPos = *(float**)pInternalCursorCoordinates;
	pCursorPos[0] = scaledCoords[0];
	pCursorPos[1] = scaledCoords[1];
}