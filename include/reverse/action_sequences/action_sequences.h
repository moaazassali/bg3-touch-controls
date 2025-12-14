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
	std::array<ActionRequest, 1> getWorldSwipeToggleSequence(bool isStart);
	std::array<ActionRequest, 2> getWorldSwipeLeftSequence(float scaledValue, float rawValue, bool isPressed);
	std::array<ActionRequest, 2> getWorldSwipeRightSequence(float scaledValue, float rawValue, bool isPressed);
}