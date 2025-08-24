#include <windows.h>
#include <InteractionContext.h>
#include <map>
#include <mutex>

extern WNDPROC g_originalWndProc;
extern HINTERACTIONCONTEXT g_hInteractionContext;

std::map<UINT32, POINTER_INFO> activePointers;
std::mutex pointersMutex;

LRESULT CALLBACK customWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_POINTERDOWN: {
		UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
		POINTER_INFO pointerInfo;
		if (GetPointerInfo(pointerId, &pointerInfo)) {
			std::lock_guard<std::mutex> lock(pointersMutex);
			activePointers[pointerId] = pointerInfo;
			AddPointerInteractionContext(g_hInteractionContext, pointerId);
		}
		return 0;
	}
	case WM_POINTERUPDATE: {
		UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
		POINTER_INFO pointerInfo;
		if (GetPointerInfo(pointerId, &pointerInfo)) {
			std::lock_guard<std::mutex> lock(pointersMutex);
			activePointers[pointerId] = pointerInfo;
		}
		return 0;
	}
	case WM_POINTERUP: {
		UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
		POINTER_INFO pointerInfo;
		// Add final update and then remove it from the interaction context and active pointers
		// If we don't do this, the last position isn't registered
		// But is that really necessary?
		// These events are sent in quick succession anyway, which is probably way faster
		// than the camera update rate of the game (the fps).
		// What if a touch happens really fast, like a tap?
		// Does that mean we will register it and then immediately remove it before the game
		// can do anything with it?
		// Maybe we should buffer multiple frames and clear them after processing in the game loop?
		// The game loop could erase all the pointers in the activePointers map that are WM_POINTERUP.
		// Needs careful testing.
		if (GetPointerInfo(pointerId, &pointerInfo)) {
			std::lock_guard<std::mutex> lock(pointersMutex);
			activePointers[pointerId] = pointerInfo;
		}
		RemovePointerInteractionContext(g_hInteractionContext, pointerId);
		{
			std::lock_guard<std::mutex> lock(pointersMutex);
			activePointers.erase(pointerId);
		}
		return 0;
	}
	}
	// Call the original window procedure for all other messages
	return CallWindowProc(g_originalWndProc, hwnd, msg, wParam, lParam);
}