#include <wtypes.h>
#include <algorithm>

#include "gesture_processor.h"
#include "utils.h"
#include "touch_handler.h"
#include "sdl_bindings/sdl_bindings.h"
#include "game_bindings.h"
#include "joystick/joystick.h"
#include "reverse/action_sequences/action_sequences.h"
#include "pending_camera_updates.h"
#include "move_internal_cursor.h"
#include "world_interaction.h"
#include "action_requests.h"


extern HWND g_gameHwnd;


void processTap(const GestureEvent& event, POINT clientPt);
void processHold(const GestureEvent& event, POINT clientPt);
void processOneContactDrag(const GestureEvent& event, POINT clientPt);
void processTwoContactDrag(const GestureEvent& event, POINT clientPt);


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
		if (event.contactCount == 1) {
			processOneContactDrag(event, clientPt);
		}
		else if (event.contactCount == 2) {
			processTwoContactDrag(event, clientPt);
		}
		break;
	default:
		std::cout << "Other gesture detected: " << event.type << std::endl;
	}
}


void processGestures() {
	auto events = TouchHandler::getSingleton().getGestureEvents();

	for (int i = 0; i < events.count; ++i) {
		const auto& event = events.events[i];
		//std::cout << "Detected gesture: " << event.type << std::endl;
		POINT clientPt = { static_cast<LONG>(event.x), static_cast<LONG>(event.y) };
		ScreenToClient(g_gameHwnd, &clientPt);
		processSingleGestureEvent(event, clientPt);
	}
}