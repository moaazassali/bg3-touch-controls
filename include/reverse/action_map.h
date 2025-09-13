#pragma once
#include <cstdint>
#include <cstddef>

#include "RE/Action.h"


namespace reverse {
	struct ActionMapNode {
		ActionMapNode* next;
		uint32_t key;
		std::byte _pad_008[4];
		Action action;
	};


	Action* getActionById(ActionMapNode** actionMap, uint32_t actionMapCount, uint32_t actionId);
}