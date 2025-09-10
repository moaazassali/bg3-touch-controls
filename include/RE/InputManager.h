#pragma once
#include <cstddef>

#include "RE/action_map.h"


struct InputManager {
	std::byte _pad_000[0x40];
	std::byte actionBufferManagerStuctStart;
	std::byte _pad_041[0x7F]; 
	uint32_t actionMapCount;
	uint32_t actionMapSize;
	ActionMapNode** actionMap;
	std::byte _pad_0D0[0x520];
	// More is known but not needed for now. Will add+document later.
};


extern InputManager* g_inputManager;