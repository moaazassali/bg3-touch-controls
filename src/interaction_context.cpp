#include <windows.h>
#include <iostream>
#include <InteractionContext.h>
#include "pending_camera_updates.h"
#include <algorithm>

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
		INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_Y
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

static void CALLBACK interactionContextCallback(void* clientData, const INTERACTION_CONTEXT_OUTPUT2* output) {
	// print interaction type
	switch (output->interactionId) {
	case INTERACTION_ID_TAP:
		//std::cout << "TAP event detected with " << output->arguments.tap.count << " taps and "
			//<< output->contactCount << " contacts." << std::endl;
		break;
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

			// Disabled for now because pitch change on one-finger Y drag feels more natural.
			// Will probably move this to rely on two-finger pinch zoom later.
			//constexpr float zoomFactor = 1.0f;
			//g_pendingCameraUpdates.zoomDelta.fetch_add(output->arguments.manipulation.delta.translationY * zoomFactor);
		}
		else if (output->contactCount == 2) {
			// Two finger manipulation. Use for panning. Make configurable.
			constexpr float xPanFactor = 1.0f;
			g_pendingCameraUpdates.xPanDelta.fetch_add(output->arguments.manipulation.delta.translationX * xPanFactor);
			constexpr float yPanFactor = 1.0f;
			g_pendingCameraUpdates.yPanDelta.fetch_add(output->arguments.manipulation.delta.translationY * yPanFactor);
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
		hResult = RegisterOutputCallbackInteractionContext2(g_hInteractionContext, interactionContextCallback, nullptr);
		if FAILED(hResult) {
			std::cout
				<< "FAILURE: Could not register interaction context output callback. HRESULT: "
				<< std::hex << hResult << std::dec << std::endl;
		}
		std::cout << "Interaction Context callback registered." << std::endl;
	}
}
