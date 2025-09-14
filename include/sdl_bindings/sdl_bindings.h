#pragma once
#include <SDL.h>


namespace sdl_bindings {
	// Assumes that MH_Initialize has already been called
	void bindToSDL();
	bool installSDLCreateWindowHook();
	uint32_t getWindowID(SDL_Window* window);

	using SDL_CreateWindow_t = SDL_Window* (*)(const char* title, int x, int y, int w, int h, Uint32 flags);
	using SDL_PushEvent_t = int(*)(SDL_Event* event);
	using SDL_GetTicks_t = Uint32(*)(void);
	using SDL_GetWindowID_t = Uint32(*)(SDL_Window* window);

	extern SDL_Window* g_gameWindow;
	extern SDL_PushEvent_t original_SDL_PushEvent;
	extern SDL_GetTicks_t original_SDL_GetTicks;
	extern SDL_GetWindowID_t original_SDL_GetWindowID;
}