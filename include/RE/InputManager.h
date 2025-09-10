#pragma once
#include <cstddef>


struct InputManager {
	std::byte _pad_000[0x40];
	std::byte actionBufferManagerStuctStart;
	std::byte _pad_041[0x5AF]; // More is known but not needed for now. Will add later.
};


extern InputManager* g_inputManager;