#include <wtypes.h>

#include "touch_handler.h"
#include "sdl_bindings/sdl_bindings.h"
#include "move_internal_cursor.h"
#include "world_interaction.h"
#include "ability_usage.h"
#include "game_camera.h"
#include "ui_bindings.h"


void processTap(const GestureEvent& event, POINT clientPt) {
	bool sendAbilityConfirmPress = false;
	if (isConfirmButtonVisible() && isPointInsideConfirmButton((float)clientPt.x, (float)clientPt.y)) {
		sendAbilityConfirmPress = true;
	}

	const auto prevEntity = getCurrentWorldTargetEntity();
	if (!sendAbilityConfirmPress) {
		moveInternalCursor((float)clientPt.x, (float)clientPt.y);
		// Not really needed but if UI element was clicked on before and has a hover effect active like
		// the details pop-up, this sends a UIMouseMove event which will clear that hover effect properly.
		// This is also very helpful in combat mode so the user can inspect enemies first without attacking.
		// XXX: ideally we should find a simple way to disable all UI hover effects because
		// they are annoying even for UI taps.
		sdl_bindings::moveCursor(clientPt.x, clientPt.y);
	}


	if (isCursorOverUI()) {
		sdl_bindings::clickMouseButton(SDL_BUTTON_LEFT);
		// The world interaction target state doesn't get updated on a UI click and maintains
		// the last target entity. Consider the case where the user taps on an entity to focus it,
		// then taps on a UI element, and then taps back on the same entity, we will end up sending
		// a click instead of just focusing on it again because prev and curr entity will match.
		// 
		// This is a consequence of our approach because we aren't resuming world interaction processing
		// when clicking on UI. But in doing so, we also introduce other issues: The state gets updated
		// with whatever entity is behind that UI element, which is fine. However, the issue happens when
		// the user then taps back at some entity. For some reason, the state doesn't get updated properly
		// and it has the previous entity instead DESPITE the new entity being focused on now with hover
		// effects and all. So this code will think we are, let's say, over ground when in fact we tapped
		// on an entity, so we send a click on the entity instead of just focusing on it again...
		// 
		// Either way, clearing the active entity interactions state will fix it.
		clearActiveWorldEntityInteractions();
		freezeProcessingWorldEntityInteractions();
		return;
	}

	// Instead of clicking immediately on any entity, I found it more UX friendly to make the
	// first click a focus/hover action, and only click on the second tap if the entity is the same
	// So, only register a tap if we were focused on an interactable entity from a previous tap
	// and the current entity from the second tap is still the same
	// XXX: should we open context menu on first tap instead of just focusing?
	resumeProcessingWorldEntityInteractions();
	updateWorldInteractions();
	const auto currEntity = getCurrentWorldTargetEntity();
	freezeProcessingWorldEntityInteractions();


	if (isConfirmButtonVisible() && !sendAbilityConfirmPress) {
		// Only send hover effect if confirm button is visible because that indicates that
		// an ability is primed, or we are in combat and can do some actions, etc...,
		// so world taps should just focus/hover for inspection/aiming purposes.
		// No clicks allowed at all. Confirm button must be used to confirm ability/move.
		return;
	}


	bool sentPressIn = false;
	if (currEntity.type != WorldEntityType::None && currEntity.type != WorldEntityType::NonInteractable && !sendAbilityConfirmPress) {
		if (currEntity.type == prevEntity.type && currEntity.id == prevEntity.id) {
			// Only send 'press in' event instead of full click immediately.
			// This fixes a bug in the world interaction system where using an ability and then clicking
			// somewhere to use it would just move the cursor there. A second click is then needed.
			// It seems like the world interaction system needs some time to process each event: moving
			// the cursor, pressing in, and pressing out. It can't handle them all at once properly.
			sdl_bindings::pressMouseButton(SDL_BUTTON_LEFT);
			sentPressIn = true;
		}
	}
	else {
		// Otherwise, the click is on the ground for character movement, over nothing, or ability confirm
		// So, we send the click unconditionally
		sdl_bindings::pressMouseButton(SDL_BUTTON_LEFT);
		sentPressIn = true;
		pressInConfirmButton();
	}

	auto pressOutTask = []() {
		sdl_bindings::releaseMouseButton(SDL_BUTTON_LEFT);
		pressOutConfirmButton();
	};

	// We need a significant delay here to address a bug in the world interaction system.
	// I only encountered this bug when using an ability that requires the character to move to the target
	// location (if target is out of range of the ability). For example, use any melee ability on a far away
	// entity that requires movement. Without moving the cursor, the click will simply cause the cursor to
	// move to the target location without actually using the ability. A second click is then needed.
	// This is in addition to the issue mentioned earlier. So without the press in and press out separation,
	// we would end up needing 3 clicks for abilities focused on a far target that initiates character movement.
	// My suspicion is that the game needs more time because such actions involve an animation that shows the
	// path the character will take to reach the target location, so it will only process the click after that
	// is done?
	// 5 frame delay mostly didn't work. 6 worked half the time. 7 was generally fine. I'll go with 12 to be safe.
	// The issue also seems independent of frame rate. I tested 10 to 120 FPS and the issue persisted similarly.
	if (sentPressIn) {
		callAfterGameProcessesCursorUpdate(pressOutTask, 12);
	}
}