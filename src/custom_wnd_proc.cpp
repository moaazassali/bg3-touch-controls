#include <windows.h>
#include <InteractionContext.h>
#include <map>
#include <mutex>
#include <chrono>
#include <iostream>

#include "reverse/action_sequences/action_sequences.h"
#include "sdl_bindings/sdl_bindings.h"
#include "touch_handler.h"


extern WNDPROC g_originalWndProc;

void processTouchInput();


static void processBufferedPacketsIfReady() {
	// We will update the camera state at most 60 times per second
	// We should probably make it configurable later and test what works best
	// Also test performance impact and accuracy of the update rate
	using clock = std::chrono::steady_clock;
	static auto lastTime = clock::now();
	auto now = clock::now();
	constexpr auto minInterval = std::chrono::duration<double>(1.0 / 60.0); // ~16.6667 ms
	if (now - lastTime >= minInterval) {
		processTouchInput();
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
			TouchHandler::getSingleton().addTouchInputEvent(TouchInputEvent{
				.id = (int)pointerId,
				.x = pointerInfo.ptPixelLocation.x,
				.y = pointerInfo.ptPixelLocation.y,
				.phase = Start,
				.timestamp = std::chrono::steady_clock::now()
				});
		}
		return 0;
	}
	case WM_POINTERUPDATE: {
		UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
		POINTER_INFO pointerInfo;
		if (GetPointerInfo(pointerId, &pointerInfo)) {
			TouchHandler::getSingleton().addTouchInputEvent(TouchInputEvent{
				.id = (int)pointerId,
				.x = pointerInfo.ptPixelLocation.x,
				.y = pointerInfo.ptPixelLocation.y,
				.phase = Update,
				.timestamp = std::chrono::steady_clock::now()
				});
		}
		processBufferedPacketsIfReady();
		return 0;
	}
	case WM_POINTERUP: {
		UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
		POINTER_INFO pointerInfo;
		if (GetPointerInfo(pointerId, &pointerInfo)) {
			TouchHandler::getSingleton().addTouchInputEvent(TouchInputEvent{
				.id = (int)pointerId,
				.x = pointerInfo.ptPixelLocation.x,
				.y = pointerInfo.ptPixelLocation.y,
				.phase = End,
				.timestamp = std::chrono::steady_clock::now()
				});
			// We must process the buffered packets here to ensure the end phase is registered
			// Otherwise, if we wait until the next getGestureEvents call in processBufferedPacketsIfReady,
			// it might not be ready yet depending on whether the next polling interval has been reached.
			// So, the next call to the processIfReady call can only happen after another input event
			// which might never happen or happen too late since we are at the end of a touch.
			// So we might suddenly get some gesture registered long after the touch has ended or not at all.
			// 
			// Maybe we should have a function that keeps running on its own thread to process buffered packets at a fixed rate
			// instead of relying on input events to trigger the processing?
			processTouchInput();
		}
		processBufferedPacketsIfReady();
		return 0;
	}
	}
	// Call the original window procedure for all other messages
	return CallWindowProc(g_originalWndProc, hwnd, msg, wParam, lParam);
}