#include "touch_handler.h"


TouchHandler::TouchInputState* TouchHandler::findStateById(int id) {
	for (int i = 0; i < MAX_STATES; ++i) {
		if (touchInputStates[i].touchId == id) {
			return &touchInputStates[i];
		}
	}
	return nullptr;
}


TouchHandler::TouchInputState* TouchHandler::findFreeState() {
	for (int i = 0; i < MAX_STATES; ++i) {
		if (!touchInputStates[i].active) {
			return &touchInputStates[i];
		}
	}
	return nullptr;
}


void TouchHandler::updateTouchInputState(TouchInputState& state, const TouchInputEvent& input) {
	// Would be helpful to have some formal state machine diagrams for this logic
	// to ensure all edge cases are covered.
	if (state.phase == End) {
		// Already ended, ignore further updates
		return;
	}

	state.phase = input.phase;
	state.currentX = input.x;
	state.currentY = input.y;

	if (input.phase == Start) {
		state.startX = input.x;
		state.startY = input.y;
		state.startTime = input.timestamp;
		state.gesture = None;
		return;
	}

	if (state.gesture != None) {
		// Gesture already recognized and state was updated
		// Nothing left to do here and we avoid mistakenly changing the gesture type
		return;
	}

	// === Tap ===
	if (abs(state.currentX - state.startX) > TAP_SLOP_PX ||
		abs(state.currentY - state.startY) > TAP_SLOP_PX) {
		state.exceededTapSlop = true;
	}

	if (!state.exceededTapSlop &&
		(input.timestamp - state.startTime) < HOLD_THRESHOLD &&
		state.phase == End) {
		state.gesture = Tap;
	}

	// === Hold ===
	if (!state.exceededTapSlop &&
		(input.timestamp - state.startTime) >= HOLD_THRESHOLD) {
		state.gesture = Hold;
		state.reachedHoldThreshold = true;
	}

	// === Drag ===
	// Everything is technically a drag including taps and holds since a touch will move
	// even if a little bit. So we set Drag as the default gesture after accounting for
	// all other gestures.
	// With the introduction of hold, we have a new possibility: a hold can transition into a drag
	// For now, we will consider that to remain a hold until the touch ends.
	if (state.exceededTapSlop) {
		state.gesture = Drag;
	}
}


void TouchHandler::addTouchInputEvent(const TouchInputEvent& input) {
	TouchInputState* state = findStateById(input.id);
	if (state) {
		updateTouchInputState(*state, input);
	}
	else {
		// New touch input, find a free state
		TouchInputState* freeState = findFreeState();
		if (freeState) {
			freeState->touchId = input.id;
			freeState->phase = input.phase;
			freeState->startX = input.x;
			freeState->startY = input.y;
			freeState->currentX = input.x;
			freeState->currentY = input.y;
			freeState->startTime = input.timestamp;
			freeState->gesture = None;
			freeState->active = true;
			freeState->exceededTapSlop = false;
			freeState->reachedHoldThreshold = false;
			freeState->sentInitialHoldEvent = false;
			freeState->lastConsumedX = input.x;
			freeState->lastConsumedY = input.y;
		}
		else {
			// MAX_STATES reached, ignore the new touch input
			// Should realistically never happen with a reasonably small time frame or large MAX_STATES
			// Could maybe think about resetting the state array to avoid a situation where no input is
			// being registered at all because all states are occupied by ended touches.
			std::cout << "No free state available for new touch input ID: " << input.id << std::endl;
		}
	}
}


TouchHandler::GestureEventsResult TouchHandler::getGestureEvents() {
	GestureEventsResult result{};
	result.count = 0;
	for (int i = 0; i < MAX_STATES; ++i) {
		TouchInputState& state = touchInputStates[i];
		if (!state.active || state.gesture == None) {
			continue;
		}

		// Hold ends when the touch ends but there are only two interesting cases:
		// 1- When the hold is recognized after the HOLD_THRESHOLD
		// 2- When the hold is released after being recognized
		// So, the hold remains in an active state and we have to ignore it until the
		// touch ends except for case 1.
		if (state.phase != End && state.gesture == Hold) {
			if (state.sentInitialHoldEvent) {
				continue;
			}
			else {
				state.sentInitialHoldEvent = true;
			}
		}

		// Common for all gestures
		result.events[result.count].touchInputId = state.touchId;
		result.events[result.count].type = state.gesture;
		result.events[result.count].phase = state.phase;
		result.events[result.count].x = state.currentX;
		result.events[result.count].y = state.currentY;
		// We only support single contact for now
		result.events[result.count].contactCount = 1;
		// Populate per-contact data
		result.events[result.count].contacts[0] = {
			.x = state.currentX,
			.y = state.currentY,
		};


		if (state.gesture == Drag) {
			DragData dragData{
				.deltaX = state.currentX - state.lastConsumedX,
				.deltaY = state.currentY - state.lastConsumedY,
			};
			// The first drag event should have a Start phase instead.
			// This can be useful if the consumer needs to run some initialization logic.
			if (state.lastConsumedX == state.startX &&
				state.lastConsumedY == state.startY &&
				state.phase == Update) {
				result.events[result.count].phase = Start;
			}
			result.events[result.count].data = dragData;
			state.lastConsumedX = state.currentX;
			state.lastConsumedY = state.currentY;

			// Update the per-contact drag data as well
			PerContactDragData perContactDragData{
				.deltaX = dragData.deltaX,
				.deltaY = dragData.deltaY,
			};
			result.events[result.count].contacts[0].data = perContactDragData;
		}
		else {
			result.events[result.count].data = std::monostate{};
		}

		result.count++;

		// All gestures end when the touch is released
		if (state.phase == End) {
			state.active = false;
		}
	}
	return result;
}