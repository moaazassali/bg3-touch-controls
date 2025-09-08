#include <iostream>
#include <vector>
#include <MinHook.h>

#include "RE/Camera.h"
#include "pending_camera_updates.h"
#include "Hooks/hooks_utils.h"
#include "Hooks/update_camera_hook.h"
#include "Hooks/calculate_pitch_hook.h"
#include "Hooks/update_camera_pitch_hook.h"
#include "Hooks/process_input_hook.h"


void installHooks()
{
	if (MH_Initialize() != MH_OK) {
		std::cout << "Failed to initialize MinHook." << std::endl;
		return;
	}

	bool success = true;

	uintptr_t start, end;
	success &= getModuleRange(start, end);
	success &= installUpdateCameraHook(start, end);
	success &= installCalculatePitchHook(start, end);
	success &= installUpdateCameraPitchHook(start, end);
	success &= installProcessInputHook(start, end);

	if (success) {
		std::cout << "All hooks installed successfully." << std::endl;
	}
	else {
		std::cout << "Some hooks failed to install." << std::endl;
		MH_Uninitialize();
	}
}