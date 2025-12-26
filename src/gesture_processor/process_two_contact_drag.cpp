#include <wtypes.h>

#include "touch_handler.h"
#include "move_internal_cursor.h"
#include "world_interaction.h"
#include "pending_camera_updates.h"
#include "reverse/action_sequences/action_sequences.h"


extern HWND g_gameHwnd;


static void handleUIScroll(float deltaY) {
	// invert for natural feel. make configurable.
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

void processTwoContactDrag(const GestureEvent& event, POINT clientPt) {
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