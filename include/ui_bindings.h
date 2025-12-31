#pragma once
#include <cstdint>


bool installUIBindingsModule(uintptr_t start, uintptr_t end);

bool isSelectedCharacterInTurnBasedMode();

enum class CharacterMode {
	Invalid,
	Idle ,
	InCombat,
	InFTB, // Free Turn-Based?
	InDialogue,
	InTrade,
};

CharacterMode getSelectedCharacterMode();

bool isSelectedCharacterCasting();

bool isSelectedCharacterMoving();

bool selectedCharacterHasAbilityPrimed();

bool isSelectedCharacterTurn();