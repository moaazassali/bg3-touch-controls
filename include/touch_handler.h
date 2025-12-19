#pragma once

#include <chrono>
#include <variant>
#include <cstdint>
#include <iostream>


enum TouchPhase {
	Start,
	Update,
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


struct DragData {
	int deltaX;
	int deltaY;
};


struct GestureEvent {
	uint32_t touchInputId;
	GestureType type;
	TouchPhase phase;
	int x;
	int y;
	std::variant<std::monostate, DragData> data;
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
		// For drag delta calculations.
		// Position of last gesture event reported, not last touch input update.
		int lastConsumedX = -1;
		int lastConsumedY = -1;
	};

	TouchInputState touchInputStates[MAX_STATES];

	// Should be set according to the actual device DPI at launch
	const static int DPI = 353;

	// px = dp * (dpi / 160)
	// Some inspiration from Android's ViewConfiguration
	// https://android.googlesource.com/platform/frameworks/base/+/master/core/java/android/view/ViewConfiguration.java
	constexpr static int TAP_SLOP = 8;
	constexpr static int TAP_SLOP_PX = TAP_SLOP * DPI / 160;
	constexpr static std::chrono::milliseconds HOLD_THRESHOLD = std::chrono::milliseconds(400);

	TouchInputState* findStateById(uint32_t id);
	TouchInputState* findFreeState();
	void updateTouchInputState(TouchInputState& state, const TouchInputEvent& input);

	// Private constructor to prevent instantiation
	TouchHandler() = default;

public:
	// Delete copy constructor and assignment operator
	TouchHandler(const TouchHandler&) = delete;
	TouchHandler& operator=(const TouchHandler&) = delete;

	// Get singleton instance
	static TouchHandler& getSingleton() {
		static TouchHandler instance;
		return instance;
	}

	struct GestureEventsResult {
		// If the time frame of checking gestures is larger than HOLD_THRESHOLD,
		// then it is possible to miss the exact moment the hold is recognized and
		// only the 2nd case gets reported.
		// This means that we can technically report a higher number of gesture events
		// than MAX_STATES since one touch state can generate more than one gesture events
		// This can also apply for drags if a drag starts and ends within the time frame, so we
		// miss the start event and only report the end event.
		// Ultimately, given our very small time frame, this should never happen in practice.
		GestureEvent events[MAX_STATES];
		int count;
	};

	void addTouchInputEvent(const TouchInputEvent& input);
	GestureEventsResult getGestureEvents();
};