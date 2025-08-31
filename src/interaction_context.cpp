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

static void CALLBACK interactionContextCallback(void* clientData, const INTERACTION_CONTEXT_OUTPUT2* output) {
	// print interaction type
	switch (output->interactionId) {
	case INTERACTION_ID_TAP:
		std::cout << "TAP event detected with " << output->arguments.tap.count << " taps and "
			<< output->contactCount << " contacts." << std::endl;
		break;
	case INTERACTION_ID_HOLD:
		std::cout << "HOLD event detected with " << output->contactCount << " contacts." << std::endl;
		break;
	case INTERACTION_ID_MANIPULATION:
	{
		// Let's also use x translation to rotate the camera for now
		// This should be configurable. i.e. use x translation for panning vs rotation.
		// Think about the memory ordering and atomicity here later
		// Does only atomicity matter? Is the performance benefit worth it?
		// We should make rotationFactor based on window width I think? and also configurable.
		if (output->contactCount == 1) {
			constexpr float rotationFactor = 0.1f;
			g_pendingCameraUpdates.angleDelta.fetch_add(output->arguments.manipulation.delta.translationX * rotationFactor);

			// Disabled for now because pitch change on one-finger Y drag feels more natural.
			// Will probably move this to rely on two-finger pinch zoom later.
			//constexpr float zoomFactor = 1.0f;
			//g_pendingCameraUpdates.zoomDelta.fetch_add(output->arguments.manipulation.delta.translationY * zoomFactor);

			constexpr float zoomFactor = 0.1f;
			const auto newPitch = std::clamp(
				g_pendingCameraUpdates.pitch.load() + output->arguments.manipulation.delta.translationY * zoomFactor,
				0.0f,
				85.55f
			);
			g_pendingCameraUpdates.pitch.store(newPitch);
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
