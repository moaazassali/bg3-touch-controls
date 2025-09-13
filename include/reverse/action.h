#pragma once
#include <cstdint>
#include <cstddef>


namespace reverse {
	struct Action {
		uint32_t actionId;
		std::byte _pad_004[0x2C];
		char* name;
		uint32_t nameLen;
		std::byte _pad_03C[0x1C];
	};
}