#include <wtypes.h>

#include "touch_handler.h"
#include "sdl_bindings/sdl_bindings.h"
#include "move_internal_cursor.h"
#include "world_interaction.h"


void processHold(const GestureEvent& event, POINT clientPt) {
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