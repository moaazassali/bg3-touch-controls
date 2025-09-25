#include <windows.h>
#include <iostream>

#include "game_bindings.h"


constexpr uint8_t minimumHotBarRows = 0x1; // make configurable


bool installMinimumHotbarRowsPatch(uintptr_t start, uintptr_t end) {
	constexpr const char* pattern = "48 89 5C 24 08 57 48 83 EC 20 0F B6 B9 E0 0C 00 00 48 8B D9 40 80 FF 02";
	uintptr_t addr = findPattern(pattern, start, end);
	if (addr == 0) {
		std::cout << "Failed to find minimum hotbar rows pattern address." << std::endl;
		return false;
	}

	constexpr std::size_t offset = 23;
	uint8_t* bytePtr = reinterpret_cast<uint8_t*>(addr + offset);

	// Sanity bounds check (ensure target lies inside the scanned module region)
	if (reinterpret_cast<uintptr_t>(bytePtr) < start || reinterpret_cast<uintptr_t>(bytePtr) >= end) {
		std::cout << "Patch target out of bounds." << std::endl;
		return false;
	}

	// Make the page writable, write the byte, flush icache, restore protection
	DWORD oldProtect = 0;
	if (!VirtualProtect(bytePtr, 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
		std::cout << "VirtualProtect failed: " << GetLastError() << std::endl;
		return false;
	}

	// Set new minimum hotbar rows
	*bytePtr = minimumHotBarRows;

	// Ensure CPU sees the change
	FlushInstructionCache(GetCurrentProcess(), bytePtr, 1);

	// Restore original protections
	DWORD tmp;
	VirtualProtect(bytePtr, 1, oldProtect, &tmp);

	std::cout << "Minimum hotbar rows patch applied successfully.";
	return true;
}