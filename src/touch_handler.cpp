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
		return;
	}

	// === Hold ===
	if (!state.exceededTapSlop &&
		(input.timestamp - state.startTime) >= HOLD_THRESHOLD) {
		state.gesture = Hold;
		state.reachedHoldThreshold = true;
		return;
	}

	// === Two finger drag ===
	if (state.exceededTapSlop) {
		// Check if any active touch exceeded tap slop recently and started around the
		// same time as this touch.
		for (int i = 0; i < MAX_STATES; ++i) {
			TouchInputState& otherState = touchInputStates[i];
			if (&otherState != &state &&
				otherState.active &&
				otherState.exceededTapSlop &&
				otherState.gesture == None &&
				abs(state.startTime - otherState.startTime) < MULTI_CONTACT_THRESHOLD) {
				// Found another touch that is part of a multi-contact drag
				state.gesture = Drag;
				otherState.gesture = Drag;
				state.correlationTouchId = otherState.touchId;
				otherState.correlationTouchId = state.touchId;
				return;
			}
		}
	}


	// === Drag ===
	// Everything is technically a drag including taps and holds since a touch will move
	// even if a little bit. So we set Drag as the default gesture after accounting for
	// all other gestures.
	// With the introduction of hold, we have a new possibility: a hold can transition into a drag
	// For now, we will consider that to remain a hold until the touch ends.
	if (state.exceededTapSlop &&
		(input.timestamp - state.startTime) >= MULTI_CONTACT_THRESHOLD) {
		state.gesture = Drag;
		return;
	}

	// We can only reach here if the touch exceeded tap slop and was released quickly
	// before MULTI_CONTACT_THRESHOLD. In that case, we deactivate it so we don't have
	// lingering active touches with no gesture.
	// We should think about reporting this as a drag.
	// Also acts as a safe guard in case of any unforeseen edge cases.
	if (state.phase == End) {
		state.active = false;
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
			freeState->correlationTouchId = -1;
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
	// if two states are part of a multi-contact gesture, we only report one event for both
	bool foundFirstContactOfMultiContact = false;

	for (int i = 0; i < MAX_STATES; ++i) {
		TouchInputState& state = touchInputStates[i];
		auto& event = result.events[result.count];
		if (!state.active || state.gesture == None) {
			continue;
		}
		if (state.correlationTouchId != -1) {
			if (foundFirstContactOfMultiContact) {
				// Already reported the event for this multi-contact gesture
				continue;
			}
			else {
				foundFirstContactOfMultiContact = true;
			}
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
		event.touchInputId = state.touchId;
		event.type = state.gesture;
		event.phase = state.phase;
		event.x = state.currentX;
		event.y = state.currentY;

		// We only support single contact for taps and holds for now
		if (state.gesture == Tap || state.gesture == Hold) {
			event.contactCount = 1;
			event.contacts[0] = {
				.x = state.currentX,
				.y = state.currentY,
			};
		}


		if (state.gesture == Drag) {
			// If 3 drags are active with 2 of them correlated, we need to make sure we process the
			// non-correlated one as a single contact drag.
			bool processingMultiContact = foundFirstContactOfMultiContact && state.correlationTouchId != -1;

			// For drag, we can have either one or two contacts
			// One contact exists either way, so populate it first
			event.contacts[0] = {
				.x = state.currentX,
				.y = state.currentY,
				.data = PerContactDragData{
					.deltaX = state.currentX - state.lastConsumedX,
					.deltaY = state.currentY - state.lastConsumedY,
				},
			};

			TouchInputState* otherState = nullptr;
			if (processingMultiContact) {
				// find the second contact state
				otherState = findStateById(state.correlationTouchId);
				event.contacts[1] = {
					.x = otherState->currentX,
					.y = otherState->currentY,
					.data = PerContactDragData{
						.deltaX = otherState->currentX - otherState->lastConsumedX,
						.deltaY = otherState->currentY - otherState->lastConsumedY,
					},
				};
			}

			event.contactCount = processingMultiContact ? 2 : 1;

			// If two contacts, we need to get their center
			event.x = processingMultiContact ?
				(state.currentX + event.contacts[1].x) / 2 :
				state.currentX;
			event.y = processingMultiContact ?
				(state.currentY + event.contacts[1].y) / 2 :
				state.currentY;

			// Populate overall drag data
			event.data = DragData{
				.deltaX = processingMultiContact ?
					((state.currentX - state.lastConsumedX) +
					(otherState->currentX - otherState->lastConsumedX)) / 2 :
					(state.currentX - state.lastConsumedX),
				.deltaY = processingMultiContact ?
					((state.currentY - state.lastConsumedY) +
					(otherState->currentY - otherState->lastConsumedY)) / 2 :
					(state.currentY - state.lastConsumedY),
				.separation = processingMultiContact ?
					(float)std::sqrt(std::pow(state.currentX - otherState->currentX, 2) +
						std::pow(state.currentY - otherState->currentY, 2)) :
					0.0f,
				.deltaSeparation = processingMultiContact ?
					(float)(std::sqrt(std::pow(state.currentX - otherState->currentX, 2) +
						std::pow(state.currentY - otherState->currentY, 2)) -
						std::sqrt(std::pow(state.lastConsumedX - otherState->lastConsumedX, 2) +
							std::pow(state.lastConsumedY - otherState->lastConsumedY, 2))) :
					0.0f,
			};

			// The first drag event should have a Start phase instead.
			// This can be useful if the consumer needs to run some initialization logic.
			if (state.lastConsumedX == state.startX &&
				state.lastConsumedY == state.startY &&
				state.phase == Update) {
				event.phase = Start;
			}

			// Update the last consumed positions
			state.lastConsumedX = state.currentX;
			state.lastConsumedY = state.currentY;
			if (processingMultiContact) {
				otherState->lastConsumedX = otherState->currentX;
				otherState->lastConsumedY = otherState->currentY;
			}
		}

		result.count++;

		// All gestures end when the touch is released
		if (state.phase == End) {
			state.active = false;
			if (foundFirstContactOfMultiContact) {
				// Also deactivate the correlated touch state
				TouchInputState* otherState = findStateById(state.correlationTouchId);
				if (otherState) {
					otherState->active = false;
				}
			}
		}
	}
	return result;
}