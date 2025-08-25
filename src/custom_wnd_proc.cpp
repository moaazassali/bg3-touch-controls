#include <windows.h>
#include <InteractionContext.h>
#include <map>
#include <mutex>
#include <chrono>
#include <iostream>

extern WNDPROC g_originalWndProc;
extern HINTERACTIONCONTEXT g_hInteractionContext;

static void processBufferedPacketsIfReady() {
	// We will update the camera state at most 60 times per second
	// We should probably make it configurable later and test what works best
	// Also test performance impact and accuracy of the update rate
	using clock = std::chrono::steady_clock;
	static auto lastTime = clock::now();
	auto now = clock::now();
	constexpr auto minInterval = std::chrono::duration<double>(1.0 / 60.0); // ~16.6667 ms
	if (now - lastTime >= minInterval) {
		if FAILED(ProcessBufferedPacketsInteractionContext(g_hInteractionContext)) {
			std::cout << "Failed to process buffered packets." << std::endl;
		}
		lastTime = now;
	}
}

LRESULT CALLBACK customWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_POINTERDOWN: {
		UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
		POINTER_INFO pointerInfo;
		if (GetPointerInfo(pointerId, &pointerInfo)) {
			if FAILED(AddPointerInteractionContext(g_hInteractionContext, pointerId)) {
				std::cout << "Failed to add pointer in WM_POINTERDOWN." << std::endl;
			}
			if FAILED(BufferPointerPacketsInteractionContext(g_hInteractionContext, 1, &pointerInfo)) {
				std::cout << "Failed to buffer pointer packet in WM_POINTERDOWN." << std::endl;
			}
		}
		processBufferedPacketsIfReady();
		return 0;
	}
	case WM_POINTERUPDATE: {
		UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
		POINTER_INFO pointerInfo;
		if (GetPointerInfo(pointerId, &pointerInfo)) {
			if FAILED(BufferPointerPacketsInteractionContext(g_hInteractionContext, 1, &pointerInfo)) {
				std::cout << "Failed to buffer pointer packet in WM_POINTERUPDATE." << std::endl;
			}
		}
		processBufferedPacketsIfReady();
		return 0;
	}
	case WM_POINTERUP: {
		UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
		POINTER_INFO pointerInfo;
		// Add final update and then remove it from the interaction context
		// If we don't do this, the last position isn't registered
		// But is that really necessary?
		// These events are sent in quick succession anyway, which is probably way faster
		// than our processing rate of the gestures (60hz now).
		// What if a touch happens really fast, like a tap?
		// Does that mean we will register it and then immediately remove it before we process it?
		// Maybe we should remove the pointer only after processing it in processBufferedPacketsIfReady?
		// Needs careful testing.
		if (GetPointerInfo(pointerId, &pointerInfo)) {
			if FAILED(BufferPointerPacketsInteractionContext(g_hInteractionContext, 1, &pointerInfo)) {
				std::cout << "Failed to buffer pointer packet in WM_POINTERUP." << std::endl;
			}
			if FAILED(RemovePointerInteractionContext(g_hInteractionContext, pointerId)) {
				std::cout << "Failed to remove pointer in WM_POINTERUP." << std::endl;
			}
		}
		processBufferedPacketsIfReady();
		return 0;
	}
	}
	// Call the original window procedure for all other messages
	return CallWindowProc(g_originalWndProc, hwnd, msg, wParam, lParam);
}