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
