#include <windows.h>
#include <iostream>

#include "game_bindings.h"


constexpr uint8_t minimumHotBarRows = 0x1; // make configurable


static bool PatchAtOffset(const char* pattern, const char* patternName, uintptr_t start, uintptr_t end, std::size_t offset, uint8_t value) {
	uintptr_t addr = findPattern(pattern, start, end);
	if (addr == 0) {
		std::cout << "Pattern not found: " << patternName << std::endl;
		return false;
	}

	uint8_t* bytePtr = reinterpret_cast<uint8_t*>(addr + offset);

	// Sanity bounds check (ensure target lies inside the scanned module region)
	if (reinterpret_cast<uintptr_t>(bytePtr) < start || reinterpret_cast<uintptr_t>(bytePtr) >= end) {
		std::cout << "Patch target out of bounds for " << patternName << std::endl;
		return false;
	}

	// Make the page writable, write the byte, flush icache, restore protection
	DWORD oldProtect = 0;
	if (!VirtualProtect(bytePtr, 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
		std::cout << "VirtualProtect failed for " << patternName << ": " << GetLastError() << std::endl;
		return false;
	}

	*bytePtr = value;

	// Ensure CPU sees the change
	FlushInstructionCache(GetCurrentProcess(), bytePtr, 1);

	// Restore original protections
	DWORD tmp;
	VirtualProtect(bytePtr, 1, oldProtect, &tmp);

	std::cout << "Patched " << patternName << " at 0x" << std::hex << (addr + offset) << std::dec << std::endl;
	return true;
}


bool installMinimumHotbarRowsPatch(uintptr_t start, uintptr_t end) {
	constexpr const char* pattern1 = "48 89 5C 24 08 57 48 83 EC 20 0F B6 B9 E0 0C 00 00 48 8B D9 40 80 FF 02";
	constexpr const char* pattern2 = "0F 84 B2 00 00 00 48 8B 87 20 04 00 00 48 85 C0 0F 84 A2 00 00 00 BF 02";

	constexpr std::size_t offset1 = 23;
	constexpr std::size_t offset2 = 23;

	bool patched1 = PatchAtOffset(pattern1, "decrementHotBarRows", start, end, offset1, minimumHotBarRows);
	// Second patch is needed because there is some hardcoded '2' value which has the side effect of resetting
	// the hotbar rows to 2 if the value is less than 2.
	bool patched2 = PatchAtOffset(pattern2, "unk_syncsSomeUIElements", start, end, offset2, minimumHotBarRows);

	if (patched1 && patched2) {
		std::cout << "Minimum hotbar rows patch applied successfully in decrementHotBarRows and unk_syncsSomeUIElements." << std::endl;
		return true;
	}

	std::cout << "Failed to patch both hotbar patterns. decrementHotBarRows=" << patched1 << " unk_syncsSomeUIElements=" << patched2 << std::endl;
	return false;
}