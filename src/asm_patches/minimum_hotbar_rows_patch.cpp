#include <windows.h>
#include <iostream>
#include <vector>

#include "game_bindings.h"


constexpr uint8_t minimumHotBarRows = 0x1; // make configurable


bool installMinimumHotbarRowsPatch(uintptr_t start, uintptr_t end) {
	constexpr const char* pattern1 = "48 89 5C 24 08 57 48 83 EC 20 0F B6 B9 E0 0C 00 00 48 8B D9 40 80 FF 02";
	constexpr const char* pattern2 = "0F 84 B2 00 00 00 48 8B 87 20 04 00 00 48 85 C0 0F 84 A2 00 00 00 BF 02";
	constexpr const char* pattern1Name = "decrementHotBarRows";
	constexpr const char* pattern2Name = "unk_syncsSomeUIElements";
	constexpr std::size_t offset1 = 23;
	constexpr std::size_t offset2 = 23;

	auto pattern1Addr = findPattern(pattern1, start, end);
	auto pattern2Addr = findPattern(pattern2, start, end);

	if (pattern1Addr == 0) {
		std::cout << "Failed to find pattern for " << pattern1Name << std::endl;
	}
	if (pattern2Addr == 0) {
		std::cout << "Failed to find pattern for " << pattern2Name << std::endl;
	}
	if (pattern1Addr == 0 || pattern2Addr == 0) {
		return false;
	}

	bool patched1 = asmPatchAtOffset(pattern1Addr, offset1, std::vector<uint8_t>{minimumHotBarRows}, pattern1Name);
	// Second patch is needed because there is some hardcoded '2' value which has the side effect of resetting
	// the hotbar rows to 2 if the value is less than 2.
	bool patched2 = asmPatchAtOffset(pattern2Addr, offset2, std::vector<uint8_t>{minimumHotBarRows}, pattern2Name);

	if (!patched1) {
		std::cout << "Failed to patch " << pattern1Name << std::endl;
	}
	if (!patched2) {
		std::cout << "Failed to patch " << pattern2Name << std::endl;
	}
	if (!patched1 && !patched2) {
		return false;
	}

	std::cout << "Successfully installed minimum hotbar rows patch." << std::endl;
	return true;
}