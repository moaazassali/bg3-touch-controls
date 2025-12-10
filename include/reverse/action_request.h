#pragma once
#include <cstddef>
#include <cstdint>

#include "reverse/action.h"


namespace reverse {
	// This is the struct used by the game to add actions to the game's
	// temporary action buffers before processing as well as the final buffer
	// to actually trigger the actions in-game.
	// The game creates these action requests in the processInput() function.
	struct ActionRequest {
		Action* action;
		uint8_t playerIndex;
		std::byte _pad_009[3];
		uint32_t actionId;
		uint16_t deviceId;
		uint8_t playerIndex_dup1;
		uint8_t actionState;
		float prevPressValue;
		float prevRawValue;
		bool prevIsPressed;
		std::byte _pad_01D[3];
		float currentPressValue;
		float currentRawValue;
		bool currentIsPressed;
		std::byte _pad_0029[7];
		uint16_t deviceId_dup1;
		uint16_t inputId;
		std::byte _pad_034[4];
	};
}