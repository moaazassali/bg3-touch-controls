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


static void handleUIScroll(float deltaY) {
	// invert for natural fell. make configurable.
	constexpr float scrollFactor = 0.1f;
	constexpr float scrollThreshold = 1.0f;

	const float abs_deltaY = std::abs(deltaY);

	if (abs_deltaY < scrollThreshold) {
		return;
	}

	auto seq = deltaY >= 0 ?
		reverse::getSwipeDownSequence(abs_deltaY * scrollFactor) :
		reverse::getSwipeUpSequence(abs_deltaY * scrollFactor);

	processActionRequests(seq);
}


static bool twoContactDragStartedOverUI = false;

static void processTwoContactDrag(const GestureEvent& event, POINT clientPt) {
	// We should also not do any camera movement when we are over UI here.
	// But how do we determine that?
	// Do we move cursor to midpoint of two contacts and check if we're over UI?
	// Or do we check if either contact is over UI? If so, how do we do that? Do we move the cursor quickly
	// to each contact and check the boolean?
	// Will there be a timing issue here since the cursor position might be updated asynchronously but
	// we can only check if we're over UI synchronously here?
	// Maybe we should put a write breakpoint on the g_isCursorOverUI boolean and see how the game updates it.
	// Two finger manipulation. Use for panning. Invert for natural feel. Make configurable. 
	auto& dragData = std::get<DragData>(event.data);

	// See comment in processOneContactDrag
	clearActiveWorldEntityInteractions();
	freezeProcessingWorldEntityInteractions();

	if (event.phase == Start) {
		// Only if both contacts started over UI then we consider the two-contact drag to be over UI
		POINT clientPt1 = { event.contacts[0].x, event.contacts[0].y };
		ScreenToClient(g_gameHwnd, &clientPt1);
		moveInternalCursor((float)clientPt1.x, (float)clientPt1.y);
		bool firstOverUI = isCursorOverUI();

		POINT clientPt2 = { event.contacts[1].x, event.contacts[1].y };
		ScreenToClient(g_gameHwnd, &clientPt2);
		moveInternalCursor((float)clientPt2.x, (float)clientPt2.y);
		bool secondOverUI = isCursorOverUI();

		twoContactDragStartedOverUI = firstOverUI && secondOverUI;
	}

	if (twoContactDragStartedOverUI) {
		handleUIScroll(dragData.deltaY);
	}
	else {
		constexpr float xPanFactor = -3.0f;
		g_pendingCameraUpdates.xPanDelta.fetch_add(dragData.deltaX * xPanFactor);
		constexpr float yPanFactor = -3.0f;
		g_pendingCameraUpdates.yPanDelta.fetch_add(dragData.deltaY * yPanFactor);

		// invert for natural feel. Make configurable.
		constexpr float zoomFactor = -5.0f;
		g_pendingCameraUpdates.zoomDelta.fetch_add(dragData.deltaSeparation * zoomFactor);
	}
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