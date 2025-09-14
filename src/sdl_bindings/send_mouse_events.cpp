#include "sdl_bindings/sdl_bindings.h"


namespace sdl_bindings {
	void moveCursor(int32_t x, int32_t y, int32_t xrel, int32_t yrel) {
		SDL_Event event{};
		event.type = SDL_MOUSEMOTION;
		event.motion.x = x;
		event.motion.y = y;
		event.motion.xrel = 0; // could base this off the in-game global cursor position if needed
		event.motion.yrel = 0;
		event.motion.state = 0;
		event.motion.timestamp = original_SDL_GetTicks();
		event.motion.windowID = original_SDL_GetWindowID(g_gameWindow);
		original_SDL_PushEvent(&event);
	}


	void clickMouseButton(uint8_t button) {
		SDL_Event eventDown{};
		eventDown.type = SDL_MOUSEBUTTONDOWN;
		eventDown.button.button = button;
		eventDown.button.state = SDL_PRESSED;
		eventDown.button.clicks = 1;
		eventDown.button.timestamp = original_SDL_GetTicks();
		eventDown.button.windowID = original_SDL_GetWindowID(g_gameWindow);
		original_SDL_PushEvent(&eventDown);

		SDL_Event eventUp{};
		eventUp.type = SDL_MOUSEBUTTONUP;
		eventUp.button.button = button;
		eventUp.button.state = SDL_RELEASED;
		eventUp.button.clicks = 1;
		eventUp.button.timestamp = original_SDL_GetTicks();
		eventUp.button.windowID = original_SDL_GetWindowID(g_gameWindow);
		original_SDL_PushEvent(&eventUp);
	}
}