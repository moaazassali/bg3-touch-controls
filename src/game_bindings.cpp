#include <windows.h> // needed so we don't get 'GetModuleInformation undefined' error
#include <vector>
#include <iostream>
#include <psapi.h>
#include <MinHook.h>

#include "game_bindings.h"
#include "reverse/functions.h"
#include "reverse/internal_cursor_position.h"
#include "move_internal_cursor.h"
#include "world_interaction.h"
#include "action_requests.h"


reverse::processActionRequestsQueue_t original_processActionRequestsQueue = nullptr;
reverse::InternalCursorPosition** reverse::g_internalCursorPosition = nullptr;
reverse::filterPreFinalActionsQueue_t original_filterPreFinalActionsQueue = nullptr;


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


bool asmPatchAtOffset(uintptr_t address, std::size_t offset, const std::vector<uint8_t>& bytes, const char* patchName) {
	uintptr_t targetStart = address + offset;
	uintptr_t targetEndExclusive = targetStart + bytes.size();
	uint8_t* bytePtr = reinterpret_cast<uint8_t*>(targetStart);

	// Make the page writable, write the bytes, flush icache, restore protection
	DWORD oldProtect = 0;
	if (!VirtualProtect(bytePtr, static_cast<SIZE_T>(bytes.size()), PAGE_EXECUTE_READWRITE, &oldProtect)) {
		std::cout << "VirtualProtect failed for " << patchName << ": " << GetLastError() << std::endl;
		return false;
	}

	std::memcpy(bytePtr, bytes.data(), bytes.size());

	// Ensure CPU sees the change
	FlushInstructionCache(GetCurrentProcess(), bytePtr, static_cast<SIZE_T>(bytes.size()));

	// Restore original protections
	DWORD tmp;
	VirtualProtect(bytePtr, static_cast<SIZE_T>(bytes.size()), oldProtect, &tmp);

	std::cout << "Patched " << patchName << " at 0x" << std::hex << targetStart << std::dec << " (len=" << bytes.size() << ")" << std::endl;
	return true;
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

	
static void initializePointers(uintptr_t start, uintptr_t end)
{
	original_processActionRequestsQueue = (reverse::processActionRequestsQueue_t)(start + reverse::offset_processActionRequestsQueue);
	original_filterPreFinalActionsQueue = (reverse::filterPreFinalActionsQueue_t)(start + reverse::offset_filterPreFinalActionsQueue);
	reverse::g_internalCursorPosition = (reverse::InternalCursorPosition**)(start + reverse::offset_internalCursorPosition);
}


static void installHooks(uintptr_t start, uintptr_t end)
{
	bool success = true;
	success &= installUpdateCameraHook(start, end);
	success &= installCalculatePitchHook(start, end);
	success &= installUpdateCameraPitchHook(start, end);
	success &= installMinimumHotbarRowsPatch(start, end);
	success &= installRegisterHotBarUIBindingsHook(start, end);
	success &= installIncreaseHotBarUIBindingHandlersStructSizePatch(start, end);
	success &= installRegisterHotBarUIBindingHandlersHook(start, end);
	//success &= installPushToActionRequestsQueueHook(start, end);
	//success &= installFilterPreFinalActionsQueueHook(start, end);
	success &= installDX11PresentHook();
	success &= installMoveCharacterHook(start, end);
	success &= installRunMoveCharacterwithoutControllerPatch(start, end);

	success &= installMoveInternalCursorModule(start, end);
	success &= installWorldInteractionModule(start, end);
	success &= installActionRequestsModule(start, end, false);

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

	initializePointers(start, end);
	installHooks(start, end);
}