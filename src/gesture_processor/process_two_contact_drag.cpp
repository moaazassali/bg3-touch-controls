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


std::array<ActionRequest, 1> getTwoContactDragUpActions(float scaledValue, float rawValue);
std::array<ActionRequest, 1> getTwoContactDragDownActions(float scaledValue, float rawValue);
std::array<ActionRequest, 1> getTwoContactDragRightActions(float scaledValue, float rawValue);
std::array<ActionRequest, 1> getTwoContactDragLeftActions(float scaledValue, float rawValue);


constexpr float scalingFactor = 0.1f;

constexpr float minMovementThresholdInCm = 0.5f;
static float accumulatedDeltaX = 0.0f;
static float accumulatedDeltaY = 0.0f;
static bool exceededMinMovementThreshold = false;
// TODO: If camera is already unlocked, we don't need to check for min movement threshold
// TODO: If joystick is active, do not pan regardless of threshold to safe guard against
// cases where threshold is not enough even though the user most likely intends to zoom
// since they are using joystick for movement.
// This means that we are assuming joystick movement cannot be used with camera panning.
// I think that is a reasonable assumption because the user can still tap the screen to
// move the character when the camera is 'unlocked'.
// This is similar to how the game handles character movement while camera is unlocked
// with controller -- the user can only click to move the character, not use the joystick.

static void handleOverWorldDrag(const GestureEvent& event) {

	const auto& dragData = std::get<DragData>(event.data);
	const bool isDraggingRight = dragData.deltaX > 0;
	const bool isDraggingUp = dragData.deltaY < 0;
	const float strengthX = std::abs(dragData.deltaX) * scalingFactor;
	const float strengthY = std::abs(dragData.deltaY) * scalingFactor;

	accumulatedDeltaX += dragData.deltaX;
	accumulatedDeltaY += dragData.deltaY;

	const auto distanceFromStart = std::sqrt(
		accumulatedDeltaX * accumulatedDeltaX +
		accumulatedDeltaY * accumulatedDeltaY
	);

	// Check if center moved enough to activate camera panning
	// Otherwise, user most likely intends to zoom without panning
	// So we shouldn't start a pan so that the camera keeps following the player
	// instead of being "unlocked"
	if (distanceFromStart >= TouchHandler::cmToPixels(minMovementThresholdInCm)) {
		exceededMinMovementThreshold = true;
	}

	if (exceededMinMovementThreshold) {
		if (isDraggingRight) {
			auto seq = getTwoContactDragRightActions(strengthX, strengthX);
			processActionRequests(seq);
		}
		else {
			auto seq = getTwoContactDragLeftActions(strengthX, strengthX);
			processActionRequests(seq);
		}

		if (isDraggingUp) {
			auto seq = getTwoContactDragUpActions(strengthY, strengthY);
			processActionRequests(seq);
		}
		else {
			auto seq = getTwoContactDragDownActions(strengthY, strengthY);
			processActionRequests(seq);
		}
	}

	if (event.phase == End) {
		if (exceededMinMovementThreshold) {
			// Send zero strength to end the actions in case the End event had non-zero delta
			auto seqX = getTwoContactDragRightActions(0.0f, 0.0f);
			processActionRequests(seqX);
			auto seqY = getTwoContactDragUpActions(0.0f, 0.0f);
			processActionRequests(seqY);
		}
		accumulatedDeltaX = 0.0f;
		accumulatedDeltaY = 0.0f;
		exceededMinMovementThreshold = false;
	}
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
	const auto& dragData = std::get<DragData>(event.data);

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
		handleOverWorldDrag(event);
		// invert for natural feel. Make configurable.
		constexpr float zoomFactor = -5.0f;
		g_pendingCameraUpdates.zoomDelta.fetch_add(dragData.deltaSeparation * zoomFactor);
	}
}