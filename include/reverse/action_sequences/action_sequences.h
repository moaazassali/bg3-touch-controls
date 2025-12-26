#pragma once
#include <array>
#include <span>

#include "game_bindings.h"
#include "action_requests.h"


namespace reverse {
	std::array<ActionRequest, 4> getSwipeUpSequence(float strength);
	std::array<ActionRequest, 4> getSwipeDownSequence(float strength);
	std::array<ActionRequest, 1> getWorldSwipeToggleSequence(bool isStart);
	std::array<ActionRequest, 2> getWorldSwipeLeftSequence(float scaledValue, float rawValue, bool isPressed);
	std::array<ActionRequest, 2> getWorldSwipeRightSequence(float scaledValue, float rawValue, bool isPressed);
}