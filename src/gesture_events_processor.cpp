#include <wtypes.h>

#include "utils.h"
#include "touch_handler.h"
#include "sdl_bindings/sdl_bindings.h"
#include "game_bindings.h"
#include "joystick/joystick.h"
#include "reverse/action_sequences/action_sequences.h"


extern HWND g_gameHwnd;


static void processTap(const GestureEvent& event, POINT clientPt) {
	sdl_bindings::moveCursor(clientPt.x, clientPt.y);
	// Sending a click immediately doesn't always work
	// For example, if we were interacting with a UI and lifted our finger without leaving the UI element,
	// the "cursor" would still be over the UI obviously.
	// Now consider the case where we want to tap outside the UI element (i.e. the game world) to move the character.
	// You would then assume that calling moveCursor followed by clickMouseButton would work, but it doesn't.
	// You have to instead click outside first to shift the "focus" away and then click again to register the click.
	// The moveCursor and clickMouseButton calls are processed in the same frame, so I think that the game world object
	// still thinks the cursor is over the UI element when processing the clickMouseButton call.
	// This makes it ignore the click because it thinks that the cursor is outside its bounds, so it doesn't process it.
	// By adding a small delay, we ensure that the moveCursor call is processed in a separate frame before the clickMouseButton call.
	// 
	// This applies to all interaction types, so we will use this approach for all of them when needed.
	// With this, we can be sure that the cursor position is exactly where we want it to be before sending any new events.
	// I tested down to 100ms and it stil worked fine. We are probably limited by the precision of the timer anyway.
	// Also, anything under 1/60s (16.67ms) doesn't make sense because we are processing input at 60Hz.
	// However, we will set it to 10ms to be consistent with the single finger manipulation case below.
	//
	// We will also avoid a lot frustrating and hard-to-debug issues this way where events don't seem to register.
	// We could make the delay configurable later if needed.
	//
	// What if we send the mouse click in the processInput hook after calling the original function?
	// Does this guarantee that the cursor position is updated correctly before processing the click?
	// Doesn't sending move cursor and then a mouse click technically achieve the same thing anyway?
	auto task = []() {sdl_bindings::clickMouseButton(SDL_BUTTON_LEFT);};
	scheduleTask(task, std::chrono::milliseconds(10));
}


static void processHold(const GestureEvent& event, POINT clientPt) {
	// A hold gesture triggers two events: one when the hold is recognized (after a delay)
	// and another when the finger is lifted.
	// We don't want to send another click when the finger is lifted.
	if (event.phase == Update) {
		sdl_bindings::moveCursor(clientPt.x, clientPt.y);
		auto task = []() {sdl_bindings::clickMouseButton(SDL_BUTTON_RIGHT);};
		scheduleTask(task, std::chrono::milliseconds(10));
	}
}


static void handleCameraRotation(int deltaX, TouchPhase phase) {
	constexpr float rotationScaleFactor = 0.5f;
	auto isSwipingRight = deltaX >= 0;
	auto strength = std::abs(deltaX * rotationScaleFactor);

	if (phase == Start) {
		auto seq3 = reverse::getWorldSwipeToggleSequence(true);
		reverse::populateActionPointers(seq3);
		reverse::pushSequenceToBuffer(seq3, 0);
	}
	else if (phase == End) {
		auto seq1 = reverse::getWorldSwipeToggleSequence(false);
		reverse::populateActionPointers(seq1);
		reverse::pushSequenceToBuffer(seq1, 0);
		// Send zero-strength releases to stop any ongoing swipe actions in case last
		// gesture event had a non-zero deltaX.
		auto seq2 = reverse::getWorldSwipeRightSequence(0.0f, 0.0f, false);
		reverse::populateActionPointers(seq2);
		reverse::pushSequenceToBuffer(seq2, 0);
		auto seq3 = reverse::getWorldSwipeLeftSequence(0.0f, 0.0f, false);
		reverse::populateActionPointers(seq3);
		reverse::pushSequenceToBuffer(seq3, 0);
	}
	else {
		auto seq = isSwipingRight ?
			reverse::getWorldSwipeRightSequence(strength, deltaX, false) :
			reverse::getWorldSwipeLeftSequence(strength, deltaX, false);
		reverse::populateActionPointers(seq);
		reverse::pushSequenceToBuffer(seq, 0);
	}

	// Send a hardware event to ensure the game processes the action requests queue.
	// This happens in processInput; we need it to ensure it doesn't return early.
	sdl_bindings::sendDummyF24KeyUp();
}


static bool dragStartedOverUI = false;
// We need to know which drag event is for joystick/camera
static uint32_t joystickTouchInputId = 0;
static uint32_t cameraTouchInputId = 0;

static void processDrag(const GestureEvent& event, POINT clientPt) {
	// We need to differentiate between drags done over UI or over the game world.
	// We can use a game global struct that tracks whether the cursor is over UI or not.
	// Similar to the move cursor -> click delay needed, we need to move the cursor first
	// and then wait a bit for the boolean we're interested in to be updated.
	// We need to move the cursor at the beginning just to determine if we *started* over UI.
	// Cursor movement is otherwise not needed for non-UI interactions and actually causes
	// unwanted behavior like item/npc hover effects.
	if (event.phase == Start) {
		sdl_bindings::moveCursor(clientPt.x, clientPt.y);
		dragStartedOverUI = false;
	}
	if (dragStartedOverUI) {
		sdl_bindings::moveCursor(clientPt.x, clientPt.y);
	}

	auto task = [event, clientPt]() {
		// Now that we have waited a bit, we can check if we started over UI.
		if (event.phase == Start) {
			if (*reverse::g_isCursorOverUI) {
				dragStartedOverUI = true;
			}
			else {
				// Move cursor offscreen to avoid hover effects due to our moveCursor call above.
				sdl_bindings::moveCursor(-10000, -10000);

				// Joystick activation area is left half of the screen
				auto windowSize = getWindowSizePx(g_gameHwnd);
				if (clientPt.x < windowSize.cx / 2) {
					auto& dragData = std::get<DragData>(event.data);
					activateJoystick((float)(clientPt.x - dragData.deltaX), (float)(clientPt.y - dragData.deltaY));
					joystickTouchInputId = event.touchInputId;
				}
				else {
					cameraTouchInputId = event.touchInputId;
				}
			}
		}

		if (!dragStartedOverUI) {
			auto& dragData = std::get<DragData>(event.data);
			if (g_joystickState.isActive && event.touchInputId == joystickTouchInputId) {
				moveJoystick(dragData.deltaX, dragData.deltaY);
			}
			if (event.touchInputId == cameraTouchInputId) {
				handleCameraRotation(dragData.deltaX, event.phase);
			}
		}

		if (event.phase == End) {
			if (event.touchInputId == joystickTouchInputId) {
				releaseJoystick();
				joystickTouchInputId = 0;
			}
			if (event.touchInputId == cameraTouchInputId) {
				cameraTouchInputId = 0;
			}
			dragStartedOverUI = false;
		}
		};

	scheduleTask(task, std::chrono::milliseconds(10));
}


static void processSingleGestureEvent(const GestureEvent& event, POINT clientPt) {
	switch (event.type)
	{
	case Tap:
		processTap(event, clientPt);
		break;
	case Hold:
		processHold(event, clientPt);
		break;
	case Drag:
		processDrag(event, clientPt);
		break;
	default:
		std::cout << "Other gesture detected: " << event.type << std::endl;
	}
}


void processTouchInput() {
	auto events = TouchHandler::getSingleton().getGestureEvents();

	for (int i = 0; i < events.count; ++i) {
		const auto& event = events.events[i];
		//std::cout << "Detected gesture: " << event.type << std::endl;
		POINT clientPt = { static_cast<LONG>(event.x), static_cast<LONG>(event.y) };
		ScreenToClient(g_gameHwnd, &clientPt);
		processSingleGestureEvent(event, clientPt);
	}
}