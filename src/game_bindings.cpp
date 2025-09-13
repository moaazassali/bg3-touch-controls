#include <windows.h> // needed so we don't get 'GetModuleInformation undefined' error
#include <vector>
#include <iostream>
#include <psapi.h>
#include <MinHook.h>

#include "game_bindings.h"
#include "reverse/functions.h"


//reverse::getActionRequestsBuffer_t original_getActionRequestsBuffer = nullptr;
//reverse::pushToActionRequestsQueue_t original_pushToActionRequestsQueue = nullptr;
//reverse::processActionRequestsQueue_t original_processActionRequestsQueue = nullptr;


// Simple pattern scanner created by an LLM
uintptr_t findPattern(const char* pattern, uintptr_t start, uintptr_t end)
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
bool getModuleRange(uintptr_t& start, uintptr_t& end)
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


bool installHook(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal, const char* hookName)
{
	if (MH_CreateHook(pTarget, pDetour, ppOriginal) != MH_OK)
	{
		std::cout << "Failed to create " << hookName << " hook." << std::endl;
		return false;
	}
	if (MH_EnableHook(pTarget) != MH_OK)
	{
		printf("Failed to enable %s hook\n", hookName);
		return false;
	}
	std::cout << hookName << " hook installed successfully." << std::endl;
	return true;
}


static void initializeFunctionPointers(uintptr_t start, uintptr_t end)
{
	//original_getActionRequestsBuffer = (reverse::getActionRequestsBuffer_t)(start + reverse::offset_getActionRequestsBuffer);
	//original_pushToActionRequestsQueue = (reverse::pushToActionRequestsQueue_t)(start + reverse::offset_pushToActionRequestsQueue);
	//original_processActionRequestsQueue = (reverse::processActionRequestsQueue_t)(start + reverse::offset_processActionRequestsQueue);
}


static void installHooks(uintptr_t start, uintptr_t end)
{
	if (MH_Initialize() != MH_OK) {
		std::cout << "Failed to initialize MinHook." << std::endl;
		return;
	}

	bool success = true;
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


void bindToGame() {
	uintptr_t start, end;
	if (!getModuleRange(start, end)) {
		return;
	}

	initializeFunctionPointers(start, end);
	installHooks(start, end);
}