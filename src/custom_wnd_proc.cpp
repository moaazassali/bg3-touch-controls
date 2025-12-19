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
extern HINTERACTIONCONTEXT g_hInteractionContext;
extern bool g_hasManipulationUpdateFired;
extern INTERACTION_ID g_lastInteractionId;


// Refactor: Extract common logic with interaction_context.cpp
static void handleCameraRotation(float deltaX, INTERACTION_FLAGS interactionFlags) {
	constexpr float rotationScaleFactor = 0.25f;
	auto isSwipingRight = deltaX >= 0;
	auto strength = std::abs(deltaX * rotationScaleFactor);

	if (interactionFlags & INTERACTION_FLAG_BEGIN) {
		auto seq3 = reverse::getWorldSwipeToggleSequence(true);
		reverse::populateActionPointers(seq3);
		reverse::pushSequenceToBuffer(seq3, 0);
	}
	else if (interactionFlags & INTERACTION_FLAG_END || interactionFlags & INTERACTION_FLAG_CANCEL) {
		auto seq1 = reverse::getWorldSwipeToggleSequence(false);
		reverse::populateActionPointers(seq1);
		reverse::pushSequenceToBuffer(seq1, 0);
		auto seq2 = reverse::getWorldSwipeRightSequence(0.0f, 0.0f, false);
		reverse::populateActionPointers(seq2);
		reverse::pushSequenceToBuffer(seq2, 0);
		auto seq3 = reverse::getWorldSwipeLeftSequence(0.0f, 0.0f, false);
		reverse::populateActionPointers(seq3);
		reverse::pushSequenceToBuffer(seq3, 0);
	}
	else {
		auto seq = isSwipingRight ?
			reverse::getWorldSwipeRightSequence(strength, deltaX, false) :
			reverse::getWorldSwipeLeftSequence(strength, deltaX, false);
		reverse::populateActionPointers(seq);
		reverse::pushSequenceToBuffer(seq, 0);
	}

	// Similar to the scroll case, we need to send a hardware event to ensure the game processes the action requests queue.
	sdl_bindings::sendDummyF24KeyUp();
}


static void processBufferedPacketsIfReady() {
	// We will update the camera state at most 60 times per second
	// We should probably make it configurable later and test what works best
	// Also test performance impact and accuracy of the update rate
	using clock = std::chrono::steady_clock;
	static auto lastTime = clock::now();
	auto now = clock::now();
	constexpr auto minInterval = std::chrono::duration<double>(1.0 / 60.0); // ~16.6667 ms
	if (now - lastTime >= minInterval) {
		g_hasManipulationUpdateFired = false;

		if FAILED(ProcessBufferedPacketsInteractionContext(g_hInteractionContext)) {
			std::cout << "Failed to process buffered packets." << std::endl;
		}

		if (g_lastInteractionId == INTERACTION_ID_MANIPULATION && !g_hasManipulationUpdateFired) {
			// No manipulation update was fired in this processing cycle despite the last interaction being a manipulation.
			// This indicates that the manipulation is ongoing but without movement
			// (i.e., the user is holding their finger(s) still on the screen).
			// We need to handle this case to send zero deltas to stop any ongoing movements.
			handleCameraRotation(0.0f, INTERACTION_FLAG_NONE);
		}
		lastTime = now;

		auto gestures = TouchHandler::getSingleton().getGestureEvents();
		for (int i = 0; i < gestures.count; ++i) {
			std::cout << "Detected gesture: " << gestures.events[i].type << std::endl;
		}
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

			TouchHandler::getSingleton().addTouchInputEvent(TouchInputEvent{
				.id = pointerId,
				.x = pointerInfo.ptPixelLocation.x,
				.y = pointerInfo.ptPixelLocation.y,
				.phase = Start,
				.timestamp = std::chrono::steady_clock::now()
				});
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

			TouchHandler::getSingleton().addTouchInputEvent(TouchInputEvent{
				.id = pointerId,
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
		// Add final update and then remove it from the interaction context
		// If we don't do this, the last position isn't registered
		// But is that really necessary?
		// These events are sent in quick succession anyway, which is probably way faster
		// than our processing rate of the gestures (60hz now).
		// What if a touch happens really fast, like a tap?
		// Does that mean we will register it and then immediately remove it before we process it?
		// Maybe we should remove the pointer only after processing it in processBufferedPacketsIfReady?
		// Needs careful testing.
		// 
		// UPDATE:
		// As expected, no interactions are being detected if we call RemovePointerInteractionContext here.
		// The issue isn't even just for quick taps, but also for longer touches.
		// Because we call RemovePointerInteractionContext immediately after buffering the packet,
		// the interaction context doesn't have a chance to process the buffered packets before the pointer is
		// removed from the set of pointers it tracks. This results in no interactions being detected at all.
		// So actually, ANY interaction that relies on WM_POINTERUP to signal the end of the interaction
		// will fail to be detected.
		if (GetPointerInfo(pointerId, &pointerInfo)) {
			if FAILED(BufferPointerPacketsInteractionContext(g_hInteractionContext, 1, &pointerInfo)) {
				std::cout << "Failed to buffer pointer packet in WM_POINTERUP." << std::endl;
			}
			// Force processing of buffered packets to ensure the interaction is recognized
			// This will not be called too often, only on pointer up events
			// so there should be no performance issues
			// As fast as the user can tap, it will never be more than 10-20 times a second which
			// is negligible compared to our 60hz update rate.
			ProcessBufferedPacketsInteractionContext(g_hInteractionContext);
			if FAILED(RemovePointerInteractionContext(g_hInteractionContext, pointerId)) {
				std::cout << "Failed to remove pointer in WM_POINTERUP." << std::endl;
			}

			TouchHandler::getSingleton().addTouchInputEvent(TouchInputEvent{
				.id = pointerId,
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
			auto gestures = TouchHandler::getSingleton().getGestureEvents();
			for (int i = 0; i < gestures.count; ++i) {
				std::cout << "Detected gesture: " << gestures.events[i].type << std::endl;
			}
		}
		processBufferedPacketsIfReady();
		return 0;
	}
	}
	// Call the original window procedure for all other messages
	return CallWindowProc(g_originalWndProc, hwnd, msg, wParam, lParam);
}