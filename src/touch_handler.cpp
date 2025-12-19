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


struct TouchInputState {
	uint32_t touchId;
	TouchPhase phase;
	GestureType gesture;
	bool active;
	int startX;
	int startY;
	int currentX;
	int currentY;
	std::chrono::steady_clock::time_point startTime;
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
	TouchInputState touchInputStates[MAX_STATES];

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
		if (input.phase == Start) {
			state.startX = input.x;
			state.startY = input.y;
			state.startTime = input.timestamp;
			state.gesture = None;
		}

		state.phase = input.phase;
		state.currentX = input.x;
		state.currentY = input.y;

		// TODO: Implement gesture recognition logic here
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
			}
			else {
				// MAX_STATES reached, ignore the new touch input
			}
		}
	}
};