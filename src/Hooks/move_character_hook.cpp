#include <iostream>

#include "game_bindings.h"
#include "reverse/functions.h"
#include "joystick/joystick.h"


reverse::moveCharacter_t original_moveCharacter = nullptr;
static bool init = false;
static bool initSuccess = false;
static uintptr_t targetAddr = 0;
static uintptr_t globalPointerAddr = 0;
static int32_t overrideFlagOffset = 0;
static int32_t xValueOffset = 0;
static int32_t yValueOffset = 0;

bool g_gameWorldLoaded = false;


static void* detour_moveCharacter(void* a1, void* a2, void* a3)
{
	// The function has code that looks like this right at the start:
	// pseudo code:
	// v3 = qword_7FF6D8E05220;
	// v5 = 0.0;
	// v6 = 0.0;
	// if (*(qword_7FF6D8E07C68 + 4972))
	// {
	//   v5 = *(qword_7FF6D8E07C68 + 4964);
	//	 v6 = *(qword_7FF6D8E07C68 + 4968);
	// }
	// asm:
	// .text:00007FF6D70212E0        mov     rax, cs:qword_7FF6D8E07C68
	// .text:00007FF6D70212E7        movsx   rbx, dx
	// .text:00007FF6D70212EB        cmp     byte ptr[rax + 136Ch], 0
	// .text:00007FF6D70212F2        jz      short loc_7FF6D702130B
	// .text:00007FF6D70212F4        movss   xmm9, dword ptr[rax + 1364h]
	// .text:00007FF6D70212FD        movss   xmm10, dword ptr[rax + 1368h]
	//
	// This flag seems to be false and we can override it temporarily to true and set
	// our desired movement values so that the character moves as intended.
	// Otherwise, the function queries InputManager for the controller left stick state

	if (!init) {
		constexpr const char* globalPointerPattern = "48 8B 05 ?? ?? ?? ?? 48 0F BF DA";
		constexpr const char* overrideFlagOffsetPattern = "80 B8 ?? ?? ?? ?? 00 74";
		constexpr const char* overrideValuesOffsetPattern = "F3 44 0F 10 88 ?? ?? ?? ?? F3 44 0F 10 90 ?? ?? ?? ??";

		uintptr_t globalPointerMatchAddr = findPattern(globalPointerPattern, targetAddr, targetAddr + 0x1000);
		int32_t globalPointerOffset = *reinterpret_cast<int32_t*>(globalPointerMatchAddr + 3);
		globalPointerAddr = globalPointerMatchAddr + 7 + globalPointerOffset;

		uintptr_t overrideFlagOffsetAddr = findPattern(overrideFlagOffsetPattern, targetAddr, targetAddr + 0x1000);
		overrideFlagOffset = *reinterpret_cast<int32_t*>(overrideFlagOffsetAddr + 2);

		uintptr_t overrideValuesOffsetAddr = findPattern(overrideValuesOffsetPattern, targetAddr, targetAddr + 0x1000);
		xValueOffset = *reinterpret_cast<int32_t*>(overrideValuesOffsetAddr + 5);
		yValueOffset = *reinterpret_cast<int32_t*>(overrideValuesOffsetAddr + 14);

		if (globalPointerAddr && overrideFlagOffset != 0 && xValueOffset != 0 && yValueOffset != 0) {
			initSuccess = true;
		}
		else {
			initSuccess = false;
		}

		init = true;
		g_gameWorldLoaded = true;
	}

	// Only override if joystick is active
	// This allows normal controller input to work when joystick is not used
	// because we are otherwise overriding the value with zeroes
	if (!initSuccess || !g_joystickState.isActive) {
		return original_moveCharacter(a1, a2, a3);
	}

	uintptr_t globalPtr = *reinterpret_cast<uintptr_t*>(globalPointerAddr);
	bool* pFlag = reinterpret_cast<bool*>(globalPtr + overrideFlagOffset);
	float* pX = reinterpret_cast<float*>(globalPtr + xValueOffset);
	float* pY = reinterpret_cast<float*>(globalPtr + yValueOffset);

	// Backup original values
	bool originalFlag = *pFlag;
	float originalX = *pX;
	float originalY = *pY;

	// Override with joystick values
	*pFlag = true;
	*pX = g_joystickState.strengthX;
	*pY = g_joystickState.strengthY;

	// move character with overridden values
	void* result = original_moveCharacter(a1, a2, a3);

	// Restore original values
	*pFlag = originalFlag;
	*pX = originalX;
	*pY = originalY;

	return result;
}


bool installMoveCharacterHook(uintptr_t start, uintptr_t end)
{
	uintptr_t callAddr = findPattern(reverse::pattern_moveCharacter, start, end);
	// Calculate the target function address from the CALL instruction
	int32_t relativeOffset = *reinterpret_cast<int32_t*>(callAddr + 1);
	targetAddr = callAddr + 5 + relativeOffset;
	return installHook((void*)targetAddr, &detour_moveCharacter, (void**)&original_moveCharacter, "moveCharacter");
}


bool installRunMoveCharacterwithoutControllerPatch(uintptr_t start, uintptr_t end) {
	// The function that calls moveCharacter has a check to see if a controller is connected.
	// .text:00007FF6D7021990    mov  rbx, qword ptr [rbp+57h+var_50]
	// .text:00007FF6D7021994    mov  rsi, qword ptr [rbp+57h+var_50+8]
	// .text:00007FF6D7021998    cmp  cs:isInControllerModeFlag, 0
	// .text:00007FF6D702199F    jz   loc_7FF6D7021BCB
	constexpr const char* pattern = "48 8B 5D 07 48 8B 75 0F 80 3D ?? ?? ?? ?? 00 0F 84";
	constexpr const char* patternName = "runMoveCharacterWithoutController";
	constexpr std::size_t offset = 15;

	auto patternAddr = findPattern(pattern, start, end);

	if (patternAddr == 0) {
		std::cout << "Failed to find pattern for " << patternName << std::endl;
		return false;
	}

	bool patched1 = asmPatchAtOffset(patternAddr, offset, std::vector<uint8_t>{0x90, 0x90, 0x90, 0x90, 0x90, 0x90}, patternName);

	if (!patched1) {
		std::cout << "Failed to patch " << patternName << std::endl;
		return false;
	}

	std::cout << "Successfully installed minimum hotbar rows patch." << std::endl;
	return true;
}