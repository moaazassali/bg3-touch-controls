#include <wtypes.h>

#include "touch_handler.h"
#include "sdl_bindings/sdl_bindings.h"
#include "move_internal_cursor.h"
#include "world_interaction.h"


void processTap(const GestureEvent& event, POINT clientPt) {
	auto prevEntity = getCurrentWorldTargetEntity();
	moveInternalCursor((float)clientPt.x, (float)clientPt.y);

	// Not really needed but if UI element was clicked on before and has a hover effect active like
	// the details pop-up, this sends a UIMouseMove event which will clear that hover effect properly.
	// This is also very helpful in combat mode so the user can inspect enemies first without attacking.
	// XXX: ideally we should find a simple way to disable all UI hover effects because
	// they are annoying even for UI taps.
	sdl_bindings::moveCursor(clientPt.x, clientPt.y);

	if (isCursorOverUI()) {
		// No special consideration needed for UI taps
		sdl_bindings::clickMouseButton(SDL_BUTTON_LEFT);
		freezeProcessingWorldEntityInteractions();
		return;
	}

	resumeProcessingWorldEntityInteractions();
	auto currEntity = getCurrentWorldTargetEntity();

	bool sentPressIn = false;
	// Instead of clicking immediately on any entity, I found it more UX friendly to make the
	// first click a focus/hover action, and only click on the second tap if the entity is the same
	// So, only register a tap if we were focused on an interactable entity from a previous tap
	// and the current entity from the second tap is still the same
	if (currEntity.type != WorldEntityType::None && currEntity.type != WorldEntityType::NonInteractable) {
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
		// Otherwise, the click is on the ground for character movement or over nothing
		// So, we send the click unconditionally
		sdl_bindings::pressMouseButton(SDL_BUTTON_LEFT);
		sentPressIn = true;
	}

	auto pressOutTask = []() {
		sdl_bindings::releaseMouseButton(SDL_BUTTON_LEFT);
		freezeProcessingWorldEntityInteractions();
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