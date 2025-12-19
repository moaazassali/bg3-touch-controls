#include <windows.h>
#include <iostream>
#include <InteractionContext.h>
#include <algorithm>
#include <SDL.h>
#include <thread>
#include <functional>
#include <chrono>

#include "pending_camera_updates.h"
#include "sdl_bindings/sdl_bindings.h"
#include "reverse/input_manager.h"
#include "reverse/action_sequences/action_sequences.h"
#include "utils.h"


extern HINTERACTIONCONTEXT g_hInteractionContext;
PendingCameraUpdates g_pendingCameraUpdates;


// https://learn.microsoft.com/en-us/windows/win32/api/interactioncontext/ne-interactioncontext-interaction_id
// https://learn.microsoft.com/en-us/windows/win32/api/interactioncontext/ne-interactioncontext-interaction_configuration_flags
// Remember to update the configurationCount in SetInteractionConfigurationInteractionContext
static const INTERACTION_CONTEXT_CONFIGURATION config[] = {
	{
		INTERACTION_ID_TAP,
		INTERACTION_CONFIGURATION_FLAG_TAP |
		INTERACTION_CONFIGURATION_FLAG_TAP_DOUBLE |
		INTERACTION_CONFIGURATION_FLAG_TAP_MULTIPLE_FINGER
	},
	{
		INTERACTION_ID_HOLD,
		INTERACTION_CONFIGURATION_FLAG_HOLD |
		INTERACTION_CONFIGURATION_FLAG_HOLD_MULTIPLE_FINGER
	},
	{
		INTERACTION_ID_MANIPULATION,
		INTERACTION_CONFIGURATION_FLAG_MANIPULATION |
		INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_X |
		INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_Y |
		INTERACTION_CONFIGURATION_FLAG_MANIPULATION_SCALING
	}
};


static void handleUIScroll(int x, int y, float deltaY) {
	// invert for natural fell. make configurable.
	constexpr float scrollFactor = 0.1f;
	constexpr float scrollThreshold = 1.0f;

	const float abs_deltaY = std::abs(deltaY);

	if (abs_deltaY < scrollThreshold) {
		return;
	}

	EnterCriticalSection((CRITICAL_SECTION*)reverse::g_inputManager);

	auto seq = deltaY >= 0 ?
		reverse::getSwipeDownSequence(abs_deltaY * scrollFactor) :
		reverse::getSwipeUpSequence(abs_deltaY * scrollFactor);

	reverse::populateActionPointers(seq);
	reverse::pushSequenceToBuffer(seq, 0);
	// We need to send an input event to force the game to process the action requests queue.
	// We could technically force the processing of the action requests queue ourselves, but there is
	// a bunch of logic around that in original_processInput that we would have to replicate which doesn't
	// seem straightforward and it's too much effort to reverse engineer all that logic when we can just
	// send a dummy input event to trigger the processing.
	// 
	// In this case though, we need to send a mouse motion event anyways because the location of the game's
	// virtual cursor is important in determining whether a UI scroll should be processed.
	// For instance, if the cursor is not over a scrollable UI element, the scroll action should be ignored.
	sdl_bindings::moveCursor(x, y);
	LeaveCriticalSection((CRITICAL_SECTION*)reverse::g_inputManager);
}


static bool manipulationStartedOverUI = false;

static void CALLBACK interactionContextCallback(void* clientData, const INTERACTION_CONTEXT_OUTPUT2* output) {
	HWND hwnd = reinterpret_cast<HWND>(clientData);

	POINT clientPt = { static_cast<LONG>(output->x), static_cast<LONG>(output->y) };
	ScreenToClient(hwnd, &clientPt);

	switch (output->interactionId) {
	case INTERACTION_ID_TAP:
	{
		break;
	}
	case INTERACTION_ID_HOLD:
	{
		break;
	}
	case INTERACTION_ID_MANIPULATION:
	{
		// Let's also use x translation to rotate the camera for now
		// This should be configurable. i.e. use x translation for panning vs rotation.
		// Think about the memory ordering and atomicity here later
		// Does only atomicity matter? Is the performance benefit worth it?
		if (output->contactCount == 1) {
			// Similar to the tap case, we need a small delay for the g_isCursorOverUI boolean to be updated correctly
			// based on the new cursor position.
			// We also only need to move the cursor at the beginning to determine if we're over UI or not.
			// And then only if we're starting over UI.
			// Otherwise, for world manipulations, moving the cursor actually causes unwanted behavior like 
			// world items and npc hover effects.
			if (output->interactionFlags & INTERACTION_FLAG_BEGIN) {
				sdl_bindings::moveCursor(clientPt.x, clientPt.y);
				// Not really needed but just to be safe, reset the flag here.
				manipulationStartedOverUI = false;
			}
			if (manipulationStartedOverUI) {
				sdl_bindings::moveCursor(clientPt.x, clientPt.y);
			}

			auto translationX = output->arguments.manipulation.delta.translationX;
			auto translationY = output->arguments.manipulation.delta.translationY;
			auto interactionFlags = output->interactionFlags;

			// Technically not thread safe because of the array access, but we all our calls should always happen in order
			// one by one. So, it should be fine for now. Implement mutex later.
			auto task = [translationX, translationY, interactionFlags, clientPt]() {
				// Sending action requests of UI press and moving the cursor doesn't seem to activate the drag and drop behavior.
				// However, sending an initial mouse press makes it work, so there might be some flag somewhere that gets set
				// regarding mouse state independent of action requests, or we maybe have an issue with our UI press action
				// requests (although I couldn't find any).
				// Another possibility is that the consumers of UI press actions checks the mouse state somehow directly
				// but I couldn't find calls to SDL to check mouse state though, so they could be using some other way.
				//
				// Our only option then is to send an actual mouse press event via SDL and find a way to determine if we are
				// over a UI element or not to avoid triggering mouse events when we are not over UI like when we want to rotate
				// the camera beacuse that will cause character movement.
				// By finding the byte at offset 0x240 of the large unknown singleton passed to unk_consumesFilteredActionsQueue,
				// we can determine that.
				//
				// This also helps us avoid doing any camera movement when we are over UI.
				if (interactionFlags & INTERACTION_FLAG_BEGIN) {
					manipulationStartedOverUI = *reverse::g_isCursorOverUI;
					if (manipulationStartedOverUI) {
						sdl_bindings::pressMouseButton(SDL_BUTTON_LEFT);
					}
					else {
						// Move cursor offscreen to avoid hover effects at wherever the cursor ends up
						// being due to the initial moveCursor call at the beginning of the manipulation.
						sdl_bindings::moveCursor(-10000, -10000);
					}
				}

				if (interactionFlags & INTERACTION_FLAG_END && manipulationStartedOverUI) {
					sdl_bindings::releaseMouseButton(SDL_BUTTON_LEFT);
					// Needed to prevent moveCursor on next manipulation start
					manipulationStartedOverUI = false;
				}
				};
			// I tested 1 to 5ms and they all caused some issues where g_isCursorOverUI was true when it shouldn't be,
			// which sends a mouse click and causes player movement (i.e. mouse drag behavior on game world).
			// This seems to be an issue only after swiping over the abilities panel and not all UI elements.
			// 10 ms seems to be fine.
			// Also I think it has to be under our processing rate because otherwise we can get a manipulation update
			// before processing the task of the previous manipulation update, which maybe? cause issues.
			scheduleTask(task, std::chrono::milliseconds(10));
		}
		else if (output->contactCount == 2) {
			// We should also not do any camera movement when we are over UI here.
			// But how do we determine that?
			// Do we move cursor to midpoint of two contacts and check if we're over UI?
			// Or do we check if either contact is over UI? If so, how do we do that? Do we move the cursor quickly
			// to each contact and check the boolean?
			// Will there be a timing issue here since the cursor position might be updated asynchronously but
			// we can only check if we're over UI synchronously here?
			// Maybe we should put a write breakpoint on the g_isCursorOverUI boolean and see how the game updates it.
			// Two finger manipulation. Use for panning. Invert for natural feel. Make configurable. 
			if (!*reverse::g_isCursorOverUI) {
				constexpr float xPanFactor = -3.0f;
				g_pendingCameraUpdates.xPanDelta.fetch_add(output->arguments.manipulation.delta.translationX * xPanFactor);
				constexpr float yPanFactor = -3.0f;
				g_pendingCameraUpdates.yPanDelta.fetch_add(output->arguments.manipulation.delta.translationY * yPanFactor);

				// invert for natural feel. Make configurable.
				constexpr float zoomFactor = -5.0f;
				g_pendingCameraUpdates.zoomDelta.fetch_add(output->arguments.manipulation.delta.expansion * zoomFactor);
			}
			else {
				// Uses the center of the two contacts. Might not be ideal but it's better than nothing.
				// Let's lock the cursor to the position at the beginning of the interaction for better control.
				// This way we don't end up moving the cursor outside the UI element we're trying to scroll and
				// lose the scrolling interaction or cause other unwanted effects (like scrolling a different UI element
				// or panning the camera if we move outside the UI).
				static int initX, initY = 0;
				if (output->interactionFlags & INTERACTION_FLAG_BEGIN) {
					initX = clientPt.x;
					initY = clientPt.y;
				}
				handleUIScroll(initX, initY, output->arguments.manipulation.delta.translationY);
			}
		}
		break;
	}
	default:
		std::cout << "Other interaction event detected: " << output->interactionId << std::endl;
		break;
	}
}


void initializeInteractionContext(HWND hwnd)
{
	if (g_hInteractionContext == nullptr) {
		auto hResult = CreateInteractionContext(&g_hInteractionContext);
		if FAILED(hResult) {
			std::cout
				<< "FAILURE: Could not create Interaction Context. HRESULT: "
				<< std::hex << hResult << std::dec << std::endl;
			return;
		}
		std::cout << "Interaction Context created." << std::endl;

		hResult = SetInteractionConfigurationInteractionContext(
			g_hInteractionContext,
			3,
			config
		);
		if FAILED(hResult) {
			std::cout
				<< "FAILURE: Could not set Interaction Context configuration. HRESULT: "
				<< std::hex << hResult << std::dec << std::endl;
		}
		std::cout << "Interaction Context configuration set." << std::endl;

		hResult = SetHoldParameterInteractionContext(
			g_hInteractionContext,
			HOLD_PARAMETER_THRESHOLD_START_DELAY,
			250.0f // milliseconds; default is 500ms
		);
		if FAILED(hResult) {
			std::cout
				<< "FAILURE: Could not set Interaction Context hold start delay. HRESULT: "
				<< std::hex << hResult << std::dec << std::endl;
		}

		// Register our callback function to receive the events from the Interaction Context
		hResult = RegisterOutputCallbackInteractionContext2(g_hInteractionContext, interactionContextCallback, hwnd);
		if FAILED(hResult) {
			std::cout
				<< "FAILURE: Could not register interaction context output callback. HRESULT: "
				<< std::hex << hResult << std::dec << std::endl;
		}
		std::cout << "Interaction Context callback registered." << std::endl;
	}
}
