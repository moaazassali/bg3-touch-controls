#include <chrono>


enum TouchPhase {
	Start,
	Move,
	End,
};


// Common structure representing a touch input event from different platforms
struct TouchInputEvent {
	uint32_t id;
	int x;
	int y;
	TouchPhase phase;
	std::chrono::steady_clock::time_point timestamp;
};


enum GestureType {
	None,
	Tap,
	Hold,
	Drag,
};


struct GestureEvent {
	GestureType type;
};


class TouchHandler {
private:
	// This also implies MAX_SIMULTANEOUS_TOUCHES = 3, but it's an even stronger
	// constraint. For example, it is possible to have 6 touches within some time
	// frame, but only 3 active touches at any given moment.
	// Representing all the events for such a scenario in the given time frame
	// would require a MAX_STATES of 6 even though MAX_SIMULTANEOUS_TOUCHES is
	// still 3. So, our constraint here can be summarized as:
	// The maximum number of touches in any given time frame.
	// A smaller time frame would be the most accurate representation of
	// MAX_SIMULTANEOUS_TOUCHES, because it is highly unlikely some touch would
	// be released while a new touch is registered in a very short time frame.
	// For an infinitely small time frame, MAX_STATES is unbounded if all touch
	// information must be preserved for any MAX_SIMULTANEOUS_TOUCHES > 0.
	constexpr static int MAX_STATES = 3;
	struct TouchInputState {
		uint32_t touchId = 0;
		TouchPhase phase = Start;
		GestureType gesture = None;
		bool active = false;
		int startX = -1;
		int startY = -1;
		int currentX = -1;
		int currentY = -1;
		std::chrono::steady_clock::time_point startTime;
		// We need to remember if we ever exceeded the tap slop
		// Like if we quickly swipe far and then back to starting position
		bool exceededTapSlop = false;
		bool reachedHoldThreshold = false;
		bool sentInitialHoldEvent = false;
	};
	TouchInputState touchInputStates[MAX_STATES];

	// Should be set according to the actual device DPI at launch
	const static int DPI = 353;

	// px = dp * (dpi / 160)
	// Some inspiration from Android's ViewConfiguration
	// https://android.googlesource.com/platform/frameworks/base/+/master/core/java/android/view/ViewConfiguration.java
	constexpr static int TAP_SLOP = 8; // Although Android uses 8dp, it might need an increase for a game context.
	constexpr static int TAP_SLOP_PX = TAP_SLOP * DPI / 160;
	constexpr static std::chrono::milliseconds HOLD_THRESHOLD = std::chrono::milliseconds(400);


	TouchInputState* findStateById(uint32_t id) {
		for (int i = 0; i < MAX_STATES; ++i) {
			if (touchInputStates[i].touchId == id) {
				return &touchInputStates[i];
			}
		}
		return nullptr;
	}

	TouchInputState* findFreeState() {
		for (int i = 0; i < MAX_STATES; ++i) {
			if (!touchInputStates[i].active) {
				return &touchInputStates[i];
			}
		}
		return nullptr;
	}

	void updateTouchInputState(TouchInputState& state, const TouchInputEvent& input) {
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
		if (state.exceededTapSlop && !state.reachedHoldThreshold) {
			state.gesture = Drag;
		}
	}

public:
	void addTouchInputEvent(const TouchInputEvent& input) {
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

	struct GestureEventsResult {
		GestureEvent events[MAX_STATES];
		int count;
	};

	GestureEventsResult getGestureEvents() {
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
			// If the time frame of checking gestures is larger than HOLD_THRESHOLD,
			// then it is possible to miss the exact moment the hold is recognized and
			// only the 2nd case gets reported.
			// This means that we can technically report a higher number of gesture events
			// than MAX_STATES since one touch state can generate more than one gesture events
			// However, given our much smaller time frame compared to HOLD_THRESHOLD, this should
			// never happen in practice.
			// So, the hold remains in an active state and we have to ignore it until the touch ends
			// except for case 1.
			if (state.phase != End && state.gesture == Hold) {
				if (state.sentInitialHoldEvent) {
					continue;
				}
				else {
					state.sentInitialHoldEvent = true;
				}
			}

			result.events[result.count].type = state.gesture;
			result.count++;

			// Drag only ends when the touch ends
			// We need to keep reporting drag updates until then
			if (state.phase == End) {
				state.active = false;
			}
		}
		return result;
	}
};