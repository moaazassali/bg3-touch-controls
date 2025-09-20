#include <windows.h>
#include <iostream>
#include <InteractionContext.h>
#include <algorithm>
#include <SDL.h>
#include <thread>


#include "pending_camera_updates.h"
#include "sdl_bindings/sdl_bindings.h"
#include "reverse/input_manager.h"
#include "reverse/action_sequences/action_sequences.h"


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


// store last 5 translationX deltas to smooth out the inertia start
float lastFiveAngleDeltas[5] = { 0.0f };

static void handleAngleDeltaUpdate(const float rawDelta, const INTERACTION_FLAGS interactionFlags) {
	// We should make rotationFactor based on window width I think? and also configurable.
	constexpr float rotationFactor = 0.25f;
	// invert for natural feel. Make configurable.
	auto angleDelta = -rawDelta * rotationFactor;
	// clamp maximum angleDelta to avoid sudden large jumps
	angleDelta = std::clamp(angleDelta, -15.0f, 15.0f);

	if (interactionFlags & INTERACTION_FLAG_BEGIN) {
		// If we are starting a new manipulation, we should stop any ongoing inertia
		g_pendingCameraUpdates.angleDelta.store(0);
		g_pendingCameraUpdates.angleInertiaState.store(InertiaState::NotInInertia);
		// reset last five deltas
		for (auto& delta : lastFiveAngleDeltas) {
			delta = 0.0f;
		}
		g_pendingCameraUpdates.angleDelta.store(angleDelta);
	}
	else if (interactionFlags & INTERACTION_FLAG_END) {
		// On lifting the finger, we should rotate the camera a bit based on the velocity (i.e. add some inertia)

		// Average the last 5 deltas to smooth out the inertia start
		// This doesn't include the current angleDelta, which is good because that is often 0.0f
		float sum = 0.0f;
		for (const auto& delta : lastFiveAngleDeltas) {
			sum += delta;
		}

		constexpr float initialInertiaAngleDeltaFactor = 0.25f;
		const float angleDeltaAverage = sum / 5.0f;
		const float initialInertiaAngleDelta = angleDeltaAverage * initialInertiaAngleDeltaFactor;
		// we multiply it by both rotationFactor and initialInertiaAngleDeltaFactor to make the comparison
		// fair and independent since initialInertiaAngleDelta is already scaled by initialInertiaAngleDeltaFactor
		// and rotationFactor because angleDeltaAverage is based on angleDelta which is scaled by rotationFactor.
		// This makes the inertiaActivationThreshold basically the threshold of translationX delta we consider significant enough
		constexpr float inertiaActivationThreshold = 5.0f * rotationFactor * initialInertiaAngleDeltaFactor;

		if (initialInertiaAngleDelta > inertiaActivationThreshold || initialInertiaAngleDelta < -inertiaActivationThreshold)
		{
			// These assignments should ideally be atomic together, but it should be much faster than the frame rate anyway
			g_pendingCameraUpdates.angleDelta.store(initialInertiaAngleDelta);
			// Needed in case we start a new inertia while another is ongoing.
			// If not set, the first frame of the new inertia won't take effect since any code relying on setting the initial
			// "velocity" of the inertia to dampen it gradually won't run.
			g_pendingCameraUpdates.angleInertiaState.store(InertiaState::InertiaStart);
		}
	}
	else {
		g_pendingCameraUpdates.angleDelta.fetch_add(angleDelta);
	}

	// store last 5 translationX deltas for inertia calculation
	for (int i = 4; i > 0; --i) {
		lastFiveAngleDeltas[i] = lastFiveAngleDeltas[i - 1];
	}
	lastFiveAngleDeltas[0] = angleDelta;
}


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


static void CALLBACK interactionContextCallback(void* clientData, const INTERACTION_CONTEXT_OUTPUT2* output) {
	HWND hwnd = reinterpret_cast<HWND>(clientData);
	switch (output->interactionId) {
	case INTERACTION_ID_TAP: {
		POINT clientPt = { static_cast<LONG>(output->x), static_cast<LONG>(output->y) };
		ScreenToClient(hwnd, &clientPt);
		sdl_bindings::moveCursor(clientPt.x, clientPt.y);
		sdl_bindings::clickMouseButton(SDL_BUTTON_LEFT);
		break;
	}
	case INTERACTION_ID_HOLD:
		//std::cout << "HOLD event detected with " << output->contactCount << " contacts." << std::endl;
		break;
	case INTERACTION_ID_MANIPULATION:
	{
		// Let's also use x translation to rotate the camera for now
		// This should be configurable. i.e. use x translation for panning vs rotation.
		// Think about the memory ordering and atomicity here later
		// Does only atomicity matter? Is the performance benefit worth it?
		if (output->contactCount == 1) {
			handleAngleDeltaUpdate(output->arguments.manipulation.delta.translationX, output->interactionFlags);
			handlePitchUpdate(output->arguments.manipulation.delta.translationY, output->interactionFlags);
			POINT clientPt = { static_cast<LONG>(output->x), static_cast<LONG>(output->y) };
			ScreenToClient(hwnd, &clientPt);
			// We have to disable UI scrolling for now because it conflicts with drag and drop.
			// Scrolling disables drag and drop and drag and drop disables scrolling.
			// Also since the activation for drag and drop and scrolling is the same (a swipe motion),
			// it's not possible to distinguish between the two.
			// We can still use the scrolling bars though, but they're very narrow and hard to hit with a finger (even a pen).
			// Maybe scrolling improvement should be handled by a separate UI mod instead.
			// Or we could think of better ways to activate scrolling vs drag and drop like utilizing long press or using
			// two finger swipe for scrolling.
			// handleUIScroll(clientPt.x, clientPt.y, output->arguments.manipulation.delta.translationY);
			sdl_bindings::moveCursor(clientPt.x, clientPt.y);

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
			// This also helps us avoid doing any camera movement when we are over UI, which we should add next.
			//
			// There is something a bit wrong about this though.
			// The drag and drop behavior seems to work well in UI like inventory or map.
			// However, in UI overlays like abilities slots or character panel, it's a bit off.
			// In the abilities slots, for example, it mostly doesn't drag anything but sometimes it does?
			// For the character panel, it mostly works but sometimes it doesn't?
			// Timing issue? Add delays? Use SDL_WarpMouseInWindow instead of sending mouse move events? idk...
			static bool startedPress = false;
			if (output->interactionFlags & INTERACTION_FLAG_BEGIN) {
				if (*reverse::g_isCursorOverUI) {
					startedPress = true;
					sdl_bindings::pressMouseButton(SDL_BUTTON_LEFT);
				}
			}
			else if (output->interactionFlags & INTERACTION_FLAG_END) {
				if (startedPress) {
					startedPress = false;
					sdl_bindings::releaseMouseButton(SDL_BUTTON_LEFT);
				}
			}
		}
		else if (output->contactCount == 2) {
			// Two finger manipulation. Use for panning. Invert for natural feel. Make configurable. 
			constexpr float xPanFactor = -3.0f;
			g_pendingCameraUpdates.xPanDelta.fetch_add(output->arguments.manipulation.delta.translationX * xPanFactor);
			constexpr float yPanFactor = -3.0f;
			g_pendingCameraUpdates.yPanDelta.fetch_add(output->arguments.manipulation.delta.translationY * yPanFactor);

			// invert for natural feel. Make configurable.
			constexpr float zoomFactor = -5.0f;
			g_pendingCameraUpdates.zoomDelta.fetch_add(output->arguments.manipulation.delta.expansion * zoomFactor);
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
