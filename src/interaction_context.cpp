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
#include "joystick/joystick.h"


extern HINTERACTIONCONTEXT g_hInteractionContext;
PendingCameraUpdates g_pendingCameraUpdates;

// These two global variables are needed to detect the case where a manipulation
// is still ongoing but the contact isn't moving anymore.
// This happens when the user is holding their finger still on the screen during a manipulation.
// The interaction context won't fire any ManipulationUpdate events in this case, and we sometimes
// need to handle this case to send zero deltas to stop any ongoing movements like stopping the
// camera rotation.
bool g_hasManipulationUpdateFired = false;
INTERACTION_ID g_lastInteractionId = INTERACTION_ID_NONE;


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


const auto prevDeltasCount = 7;
const auto numberOfDeltasForInertiaInitialDelta = 3;
float prevPitchDeltas[prevDeltasCount] = { 0.0f };

static void handlePitchUpdate(float rawDelta, INTERACTION_FLAGS interactionFlags) {
	constexpr float pitchFactor = 0.25f;
	auto pitchDelta = rawDelta * pitchFactor;
	pitchDelta = std::clamp(pitchDelta, -15.0f, 15.0f);

	if (interactionFlags & INTERACTION_FLAG_BEGIN) {
		g_pendingCameraUpdates.pitchDelta.store(0);
		g_pendingCameraUpdates.pitchInertiaState.store(InertiaState::NotInInertia);
		for (auto& delta : prevPitchDeltas) {
			delta = 0.0f;
		}
		g_pendingCameraUpdates.pitchDelta.store(pitchDelta);
	}
	else if (interactionFlags & INTERACTION_FLAG_END) {
		// calcualte sum of last 3 deltas and also last 10 deltas
		float sumLastThree = 0.0f;
		for (int i = 0; i < numberOfDeltasForInertiaInitialDelta && i < prevDeltasCount; ++i) {
			sumLastThree += prevPitchDeltas[i];
		}
		float sumAll = 0.0f;
		for (int i = 0; i < prevDeltasCount; ++i) {
			sumAll += prevPitchDeltas[i];
		}

		// For initial pitch delta for inertia, a smaller amount of recent measurements are a more accurate
		// indicator of the final delta of the swipe motion.
		constexpr float inertiaInitialDeltaFactor = 0.25f;
		const float deltasAverageForInertiaInitialDelta = sumLastThree / static_cast<float>(numberOfDeltasForInertiaInitialDelta);
		const float inertiaInitialDelta = deltasAverageForInertiaInitialDelta * inertiaInitialDeltaFactor;
		// Preventing the activation of inertia is a much more deliberate action from the user than activating it.
		// So taking into account a longer history of deltas can give us a better idea of whether the user is "stopping"
		// the swipe motion or is making a short swipe.
		const float pitchDeltaAverageAll = sumAll / static_cast<float>(prevDeltasCount);
		constexpr float inertiaActivationThreshold = 8.0f * pitchFactor;

		const auto isShortSwipe = std::abs(deltasAverageForInertiaInitialDelta) > std::abs(pitchDeltaAverageAll);
		const auto isLongSwipeWithNoStop = std::abs(pitchDeltaAverageAll) > inertiaActivationThreshold;
		const auto shouldActivateInertia = isShortSwipe || isLongSwipeWithNoStop;
		if (shouldActivateInertia)
		{
			g_pendingCameraUpdates.pitchDelta.store(inertiaInitialDelta);
			g_pendingCameraUpdates.pitchInertiaState.store(InertiaState::InertiaStart);
		}
	}
	else {
		g_pendingCameraUpdates.pitchDelta.fetch_add(pitchDelta);
	}

	for (int i = prevDeltasCount - 1; i > 0; --i) {
		prevPitchDeltas[i] = prevPitchDeltas[i - 1];
	}
	prevPitchDeltas[0] = pitchDelta;
}


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


static void handleCameraRotation(float deltaX, INTERACTION_FLAGS interactionFlags) {
	constexpr float rotationScaleFactor = 0.5f;
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
		// Send zero-strength releases to stop any ongoing swipe actions
		// Doesn't hurt to be safe here.
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

static bool manipulationStartedOverUI = false;

static void CALLBACK interactionContextCallback(void* clientData, const INTERACTION_CONTEXT_OUTPUT2* output) {
	g_lastInteractionId = output->interactionId;
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
		// This is called twice for a hold: once at the beginning when the start delay threshold is reached
		// and another when the finger is lifted.
		// We don't want to send another click when the finger is lifted.
		if (output->interactionFlags & INTERACTION_FLAG_BEGIN) {
			sdl_bindings::moveCursor(clientPt.x, clientPt.y);
			auto task = []() {sdl_bindings::clickMouseButton(SDL_BUTTON_RIGHT);};
			scheduleTask(task, std::chrono::milliseconds(10));
		}
		break;
	}
	case INTERACTION_ID_MANIPULATION:
	{
		// Let's also use x translation to rotate the camera for now
		// This should be configurable. i.e. use x translation for panning vs rotation.
		// Think about the memory ordering and atomicity here later
		// Does only atomicity matter? Is the performance benefit worth it?
		if (output->contactCount == 1) {
			g_hasManipulationUpdateFired = true;
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
						activateJoystick((float)clientPt.x, (float)clientPt.y);
					}
				}

				if (!manipulationStartedOverUI) {
					if (g_joystickState.isActive) {
						moveJoystick(translationX, translationY);
					}
					else {
						handleCameraRotation(translationX, interactionFlags);
						handlePitchUpdate(translationY, interactionFlags);
					}
				}

				if (interactionFlags & INTERACTION_FLAG_END && manipulationStartedOverUI) {
					sdl_bindings::releaseMouseButton(SDL_BUTTON_LEFT);
					// Needed to prevent moveCursor on next manipulation start
					manipulationStartedOverUI = false;
				}
				if (interactionFlags & INTERACTION_FLAG_END) {
					releaseJoystick();
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
