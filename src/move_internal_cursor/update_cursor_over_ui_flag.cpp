#include <iostream>

#include "utils.h"
#include "game_bindings.h"

// a1 - Some ptr from possibly the global Noesis UI view
// a2 - Output pointer to UI element hit
// a3 - Pointer to float[2] array with x and y coordinates
// Returns: a2
// We will need to capture a1 to be able to call it directly.
// This function is called when the mouse moves but it also seems to be called
// every now and then even when the mouse is not moving.
// Like a few times on game launch and when switching from some sort of view like
// from the initial splash screens to main menu.
// a1 generally stays the same.
// I noticed though that it changes when going from initial splash screens on
// game launch to the loading screen.
// Technically, this can cause an inconsistency if we call the hit test when
// the old a1 is no longer valid in this small time frame when it changes.
// However, both screens are not interactive so it should not matter.
// TODO: Verify if the address changes in other situations too.
using noesisHitTest_t = void* (*)(void* a1, void* x, float* y);
noesisHitTest_t original_noesisHitTest = nullptr;
void* pNoesisHitTest_a1 = nullptr;


static void* detour_noesisHitTest(void* a1, void* a2, float* a3)
{
	pNoesisHitTest_a1 = a1;
	return original_noesisHitTest(a1, a2, a3);
}


bool installNoesisHitTestHook(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_noesisHitTest = "48 89 5C 24 08 48 89 74 24 10 57 B8 40 00 00 00 E8 ?? ?? ?? ?? 48 2B E0 48 8B FA 48 8B D9 8B";
	uintptr_t matchAddr = findPattern(pattern_noesisHitTest, start, end);
	return installHook((void*)matchAddr, &detour_noesisHitTest, (void**)&original_noesisHitTest, "noesisHitTestHook");
}


// a1 - Some game manager struct pointer
// Since the hit test runs on game launch, this will also run on game launch.
// So, we can capture a1 right away.
using updateCursorOverUIFlag_t = void(*)(void* a1);
updateCursorOverUIFlag_t original_updateCursorOverUIFlag = nullptr;
void* pUpdateCursorOverUIFlag_a1 = nullptr;
// This is at a1 + 384
bool* pIsCursorOverUI = nullptr;


static void detour_updateCursorOverUIFlag(void* a1)
{
	pUpdateCursorOverUIFlag_a1 = a1;
	if (a1) {
		pIsCursorOverUI = (bool*)((uintptr_t)a1 + 384);
	}
	return original_updateCursorOverUIFlag(a1);
}


bool installUpdateCursorOverUIFlagHook(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_updateCursorOverUIFlag = "40 53 55 56 57 48 83 EC 28 48 8B F9";
	uintptr_t matchAddr = findPattern(pattern_updateCursorOverUIFlag, start, end);
	return installHook((void*)matchAddr, &detour_updateCursorOverUIFlag, (void**)&original_updateCursorOverUIFlag, "updateCursorOverUIFlagHook");
}


bool updateCursorOverUIFlag(float x, float y)
{
	if (!original_noesisHitTest ||
		!original_updateCursorOverUIFlag ||
		!pNoesisHitTest_a1 ||
		!pUpdateCursorOverUIFlag_a1 ||
		!pIsCursorOverUI) {
		// assume we are over UI if we can't determine otherwise
		// safer than assuming we are in the game world
		return true;
	}

	float coords[2] = { x, y };
	void* pHitUIElement = nullptr;
	original_noesisHitTest(pNoesisHitTest_a1, &pHitUIElement, coords);

	// Set the pHitUIElement (see README)
	*(void**)((uintptr_t)pUpdateCursorOverUIFlag_a1 + 136) = pHitUIElement;
	// Update mouse coordinates cache in a1 (see README)
	float* cachedCoords = reinterpret_cast<float*>((uintptr_t)pUpdateCursorOverUIFlag_a1 + 0x190);
	cachedCoords[0] = x;
	cachedCoords[1] = y;

	original_updateCursorOverUIFlag(pUpdateCursorOverUIFlag_a1);
	return *pIsCursorOverUI;
}


bool getIsCursorOverUIFlag() {
	if (!pIsCursorOverUI) {
		return true;
	}
	return *pIsCursorOverUI;
}