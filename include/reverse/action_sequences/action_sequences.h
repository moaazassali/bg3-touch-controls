#pragma once
#include <array>
#include <span>

#include "game_bindings.h"
#include "reverse/action_request.h"


namespace reverse {
	void populateActionPointers(std::span<ActionRequest> seq);
	void pushSequenceToBuffer(std::span<ActionRequest> seq, uint8_t playerIndex);

	std::array<ActionRequest, 4> getSwipeUpSequence(float strength);
	std::array<ActionRequest, 4> getSwipeDownSequence(float strength);
}