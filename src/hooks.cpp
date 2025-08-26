#include <windows.h>
#include <iostream>
#include <Psapi.h>
#include <vector>
#include <MinHook.h>
#include "RE/Camera.h"
#include "pending_camera_updates.h"

// Typedef and pattern for the camera update function. Taken from BG3_NativeCameraTweaks:
// https://github.com/ersh1/BG3_NativeCameraTweaks/blob/876f2da98ede96d3db2884ffd9fb25687ada5ff1/src/Hooks.h
using updateCamera_t = void(*)(uint64_t a1, uint64_t a2, uint64_t a3, RE::UnkObject* a4);
updateCamera_t original_updateCamera = nullptr;

// Simple pattern scanner created by an LLM
static uintptr_t findPattern(const char* pattern, uintptr_t start, uintptr_t end)
{
	std::vector<int> bytes;
	const char* p = pattern;
	while (*p)
	{
		if (*p == ' ')
		{
			p++;
			continue;
		}
		if (*p == '?')
		{
			bytes.push_back(-1); // Wildcard
			p += 2;
		}
		else
		{
			char byte[3] = { *p, *(p + 1), 0 };
			bytes.push_back(strtol(byte, nullptr, 16));
			p += 2;
		}
	}

	for (uintptr_t i = start; i < end - bytes.size(); ++i)
	{
		bool found = true;
		for (size_t j = 0; j < bytes.size(); ++j)
		{
			if (bytes[j] != -1 && *(uint8_t*)(i + j) != bytes[j])
			{
				found = false;
				break;
			}
		}
		if (found)
			return i;
	}
	return 0;
}

// Helper to get module range created by an LLM
static bool getModuleRange(uintptr_t& start, uintptr_t& end)
{
	HMODULE hModule = GetModuleHandle(NULL);
	if (!hModule) {
		std::cout << "GetModuleHandle failed." << std::endl;
		return false;
	}

	MODULEINFO moduleInfo;
	if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(MODULEINFO))) {
		std::cout << "GetModuleInformation failed." << std::endl;
		return false;
	}

	start = reinterpret_cast<uintptr_t>(hModule);
	end = start + moduleInfo.SizeOfImage;
	return true;
}

// Based on manual testing, this is called on every frame and is tied to the frame rate.
static void detour_updateCamera(uint64_t a1, uint64_t a2, uint64_t a3, RE::UnkObject* a4)
{
	// There are two CameraObject pointers in UnkObject. Only currentCameraObject seems to work.
	// Updating desiredCameraObject2 doesn't seem to have any effect.
	if (a4 && a4->currentCameraObject) {
		auto cam = a4->currentCameraObject;
		auto angleDelta = g_pendingCameraUpdates.angleDelta.exchange(0.0f);
		// We will use the amount of angleDelta as a measure of how fast we want to rotate.
		// This makes sense since angleDelta is based on how far the user moved their finger
		// within a frame, which is a measure of speed.
		// I attempted adjusting camer angle directly first but it didn't work well.
		// The range of the angle field is from (-360, 360) according to testing.
		// But how do you represent a fast rotation with that and especially one that involves
		// possibly multiple full rotations in one frame (unlikely)?
		// Although this exact logic of trying to update angle was causing sudden camera jumps
		// for fast swipes and jittery behavior The rotation somehow didn't always change just 
		// the axis we want but seemed to affect axes?
		// Invert for natural feel. Add as a config.
		cam->currentAngleDelta = -angleDelta;
	}
	original_updateCamera(a1, a2, a3, a4);
}

void installHooks()
{
	if (MH_Initialize() != MH_OK) {
		std::cout << "Failed to initialize MinHook." << std::endl;
		return;
	}

	uintptr_t start, end;
	if (!getModuleRange(start, end)) {
		MH_Uninitialize();
		return;
	}
	uintptr_t callAddr = findPattern(
		"E8 ?? ?? ?? ?? 48 8D 8D F8 04 00 00 E8 ?? ?? ?? ?? E9 AF FD FF FF",
		start,
		end
	);
	// Calculate the target function address from the CALL instruction
	int32_t relativeOffset = *reinterpret_cast<int32_t*>(callAddr + 1);
	uintptr_t targetAddr = callAddr + 5 + relativeOffset;

	if (MH_CreateHook((void*)targetAddr, &detour_updateCamera, (void**)&original_updateCamera) != MH_OK)
	{
		std::cout << "Failed to create hook." << std::endl;
		MH_Uninitialize();
		return;
	}

	if (MH_EnableHook((void*)targetAddr) != MH_OK)
	{
		printf("Failed to enable hook\n");
		MH_Uninitialize();
		return;
	}

	std::cout << "UpdateCamera hook installed successfully." << std::endl;
}