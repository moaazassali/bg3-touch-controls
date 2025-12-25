#include <wtypes.h>
#include <algorithm>

#include "utils.h"
#include "touch_handler.h"
#include "sdl_bindings/sdl_bindings.h"
#include "game_bindings.h"
#include "joystick/joystick.h"
#include "reverse/action_sequences/action_sequences.h"
#include "pending_camera_updates.h"
#include "move_internal_cursor.h"
#include "world_interaction.h"


extern HWND g_gameHwnd;
PendingCameraUpdates g_pendingCameraUpdates;


static void processTap(const GestureEvent& event, POINT clientPt) {
	resumeProcessingWorldEntityInteractions();
	moveInternalCursor((float)clientPt.x, (float)clientPt.y);
	auto task = []() {
		sdl_bindings::clickMouseButton(SDL_BUTTON_LEFT);
		// Keep hovering over the clicked entity for better visual feedback
		freezeProcessingWorldEntityInteractions();
		};

	if (isCursorOverUI()) {
		task();
	}
	else {
		// Refer to move_internal_cursor/README.md for why we need a delay for world clicks
		callAfterGameProcessesCursorUpdate(task);
	}
}


static void processHold(const GestureEvent& event, POINT clientPt) {
	// A hold gesture triggers two events: one when the hold is recognized (after a delay)
	// and another when the finger is lifted.
	// We don't want to send another click when the finger is lifted.
	if (event.phase == Update) {
		resumeProcessingWorldEntityInteractions();
		moveInternalCursor((float)clientPt.x, (float)clientPt.y);
		auto task = []() {
			sdl_bindings::clickMouseButton(SDL_BUTTON_RIGHT);
			// Keep hovering over the clicked entity for better visual feedback
			freezeProcessingWorldEntityInteractions();
			};

		if (isCursorOverUI()) {
			task();
		}
		else {
			// Refer to move_internal_cursor/README.md for why we need a delay for world clicks
			callAfterGameProcessesCursorUpdate(task);
		}
	}
}


static void handleCameraRotation(int deltaX, TouchPhase phase) {
	constexpr float rotationScaleFactor = 0.25f; // Make configurable
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


const auto prevDeltasCount = 7;
const auto numberOfDeltasForInertiaInitialDelta = 3;
float prevPitchDeltas[prevDeltasCount] = { 0.0f };

static void handlePitchUpdate(int deltaY, TouchPhase phase) {
	constexpr float pitchFactor = 0.25f;
	auto pitchDelta = (float)deltaY * pitchFactor;
	pitchDelta = std::clamp(pitchDelta, -15.0f, 15.0f);

	if (phase == Start) {
		g_pendingCameraUpdates.pitchDelta.store(0);
		g_pendingCameraUpdates.pitchInertiaState.store(InertiaState::NotInInertia);
		for (auto& delta : prevPitchDeltas) {
			delta = 0.0f;
		}
		g_pendingCameraUpdates.pitchDelta.store(pitchDelta);
	}
	else if (phase == End) {
		// calcualte sum of last 3 deltas and also last 10 deltas
		float sumLastThree = 0.0f;
		for (int i = 0; i < numberOfDeltasForInertiaInitialDelta && i < prevDeltasCount; ++i) {
			sumLastThree += prevPitchDeltas[i];
		}
		float sumAll = 0.0f;
		for (int i = 0; i < prevDeltasCount; ++i) {
			sumAll += prevPitchDeltas[i];
		}

		// For initial pitch delta for inertia, a smaller amount of recent measurements are a more accurate
		// indicator of the final delta of the swipe motion.
		constexpr float inertiaInitialDeltaFactor = 0.25f;
		const float deltasAverageForInertiaInitialDelta = sumLastThree / static_cast<float>(numberOfDeltasForInertiaInitialDelta);
		const float inertiaInitialDelta = deltasAverageForInertiaInitialDelta * inertiaInitialDeltaFactor;
		// Preventing the activation of inertia is a much more deliberate action from the user than activating it.
		// So taking into account a longer history of deltas can give us a better idea of whether the user is "stopping"
		// the swipe motion or is making a short swipe.
		const float pitchDeltaAverageAll = sumAll / static_cast<float>(prevDeltasCount);
		constexpr float inertiaActivationThreshold = 8.0f * pitchFactor;

		const auto isShortSwipe = std::abs(deltasAverageForInertiaInitialDelta) > std::abs(pitchDeltaAverageAll);
		const auto isLongSwipeWithNoStop = std::abs(pitchDeltaAverageAll) > inertiaActivationThreshold;
		const auto shouldActivateInertia = isShortSwipe || isLongSwipeWithNoStop;
		if (shouldActivateInertia)
		{
			g_pendingCameraUpdates.pitchDelta.store(inertiaInitialDelta);
			g_pendingCameraUpdates.pitchInertiaState.store(InertiaState::InertiaStart);
		}
	}
	else {
		g_pendingCameraUpdates.pitchDelta.fetch_add(pitchDelta);
	}

	for (int i = prevDeltasCount - 1; i > 0; --i) {
		prevPitchDeltas[i] = prevPitchDeltas[i - 1];
	}
	prevPitchDeltas[0] = pitchDelta;
}


static bool dragStartedOverUI = false;
// We need to know which drag event is for joystick/camera
static int joystickTouchInputId = -1;
static int cameraTouchInputId = -1;

static void processOneContactDrag(const GestureEvent& event, POINT clientPt) {
	// We need to differentiate between drags done over UI or over the game world.
	// Drags are used in UI for things like scrollbars and drag and drop.
	// We need to move the cursor at the beginning just to determine if we *started* over UI.
	// Cursor movement is otherwise not needed for non-UI interactions and actually causes
	// unwanted behavior like item/npc hover effects.
	auto& dragData = std::get<DragData>(event.data);

	// Since we freeze hovering over world entities we tapped/held on, we need to clear that hover
	clearActiveWorldEntityInteractions();
	// Keep stopping interactions processing in case it was re-enabled by a tap/hold while dragging
	freezeProcessingWorldEntityInteractions();

	if (event.phase == Start) {
		// clientPt is current position with any initial delta, not the start position
		auto startX = clientPt.x - dragData.deltaX;
		auto startY = clientPt.y - dragData.deltaY;
		dragStartedOverUI = moveInternalCursor((float)startX, (float)startY);
		if (dragStartedOverUI) {
			// We could later investigate making UI interactions work directly with action requests
			// instead of SDL events.
			sdl_bindings::pressMouseButton(SDL_BUTTON_LEFT);
		}
		else {
			// Joystick activation area is left half of the screen
			auto windowSize = getWindowSizePx(g_gameHwnd);
			if (clientPt.x < windowSize.cx / 2 && g_gameWorldLoaded && joystickTouchInputId == -1) {
				activateJoystick((float)startX, (float)startY);
				joystickTouchInputId = event.touchInputId;
				// Send right click to disable force stop any active click-to-move character movement
				// Character movement with joystick doesn't work otherwise until the previous movement ends
				// This shouldn't cause any unintended side effects because we are not over UI and because
				// world interactions are frozen.
				sdl_bindings::clickMouseButton(SDL_BUTTON_RIGHT);
			}
			else {
				cameraTouchInputId = cameraTouchInputId == -1 ? event.touchInputId : cameraTouchInputId;
			}
		}
	}

	if (dragStartedOverUI) {
		// We can't use moveInternalCursor here because we need the UIMouseMove action requests
		// to be generated for UI drags to work. We could be sending the action requests ourselves
		// directly, but I don't think that provides any benefit.
		if (event.phase == Update) {
			sdl_bindings::moveCursor(clientPt.x, clientPt.y);
		}
	}
	else {
		if (g_joystickState.isActive && event.touchInputId == joystickTouchInputId) {
			moveJoystick(dragData.deltaX, dragData.deltaY);
		}
		if (event.touchInputId == cameraTouchInputId) {
			handleCameraRotation(dragData.deltaX, event.phase);
			// Accesses global array, but should be fine because our calls are always in order with large enough delays.
			handlePitchUpdate(dragData.deltaY, event.phase);
		}
	}

	if (event.phase == End) {
		if (event.touchInputId == joystickTouchInputId) {
			releaseJoystick();
			joystickTouchInputId = -1;
		}
		if (event.touchInputId == cameraTouchInputId) {
			cameraTouchInputId = -1;
		}
		if (dragStartedOverUI) {
			sdl_bindings::releaseMouseButton(SDL_BUTTON_LEFT);
		}
		dragStartedOverUI = false;
	}
}


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

	reverse::populateActionPointers(seq);
	reverse::pushSequenceToBuffer(seq, 0);

	sdl_bindings::sendDummyF24KeyUp();
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