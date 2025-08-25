#include <windows.h>
#include <iostream>
#include <InteractionContext.h>

extern HINTERACTIONCONTEXT g_hInteractionContext;

// https://learn.microsoft.com/en-us/windows/win32/api/interactioncontext/ne-interactioncontext-interaction_id
// https://learn.microsoft.com/en-us/windows/win32/api/interactioncontext/ne-interactioncontext-interaction_configuration_flags
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
			2,
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
