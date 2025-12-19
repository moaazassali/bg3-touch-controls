#include <wtypes.h>

#include "utils.h"
#include "touch_handler.h"
#include "sdl_bindings/sdl_bindings.h"


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

}


static void processDrag(const GestureEvent& event, POINT clientPt) {

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
		std::cout << "Detected gesture: " << event.type << std::endl;
		POINT clientPt = { static_cast<LONG>(event.x), static_cast<LONG>(event.y) };
		ScreenToClient(g_gameHwnd, &clientPt);
		processSingleGestureEvent(event, clientPt);
	}
}