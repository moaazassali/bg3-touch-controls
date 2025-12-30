#pragma once
#include <cstdint>


bool installUIBindingsModule(uintptr_t start, uintptr_t end);

bool isSelectedCharacterInTurnBasedMode();

enum class CharacterMode {
	Invalid = 0,
	Idle = 1 << 0,
	InCombat = 1 << 1,
	InFTB = 1 << 2, // Free Turn-Based?
	InDialogue = 1 << 3,
	InTrade = 1 << 4,
};

CharacterMode getSelectedCharacterMode();

bool isSelectedCharacterCasting();

bool isSelectedCharacterMoving();

bool selectedCharacterHasAbilityPrimed();