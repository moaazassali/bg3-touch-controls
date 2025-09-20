#include <iostream>
#include <MinHook.h>

#include "sdl_bindings/sdl_bindings.h"


namespace sdl_bindings {
	SDL_Window* g_gameWindow = nullptr;

	SDL_PushEvent_t original_SDL_PushEvent = nullptr;
	SDL_GetTicks_t original_SDL_GetTicks = nullptr;
	SDL_GetWindowID_t original_SDL_GetWindowID = nullptr;
	SDL_WarpMouseInWindow_t original_SDL_WarpMouseInWindow = nullptr;


	void bindToSDL()
	{
		if (!installSDLCreateWindowHook()) {
			MH_Uninitialize();
			return;
		}

		HMODULE sdl_module = GetModuleHandleA("SDL2.dll");
		if (!sdl_module) {
			std::cout << "Could not get handle to SDL2.dll" << std::endl;
			return;
		}
		original_SDL_PushEvent = (SDL_PushEvent_t)GetProcAddress(sdl_module, "SDL_PushEvent");
		if (!original_SDL_PushEvent) {
			std::cout << "Could not find SDL_PushEvent in SDL2.dll" << std::endl;
			return;
		}
		original_SDL_GetTicks = (SDL_GetTicks_t)GetProcAddress(sdl_module, "SDL_GetTicks");
		if (!original_SDL_GetTicks) {
			std::cout << "Could not find SDL_GetTicks in SDL2.dll" << std::endl;
			return;
		}
		original_SDL_GetWindowID = (SDL_GetWindowID_t)GetProcAddress(sdl_module, "SDL_GetWindowID");
		if (!original_SDL_GetWindowID) {
			std::cout << "Could not find SDL_GetWindowID in SDL2.dll" << std::endl;
			return;
		}
		original_SDL_WarpMouseInWindow = (SDL_WarpMouseInWindow_t)GetProcAddress(sdl_module, "SDL_WarpMouseInWindow");
		if (!original_SDL_WarpMouseInWindow) {
			std::cout << "Could not find SDL_WarpMouseInWindow in SDL2.dll" << std::endl;
			return;
		}

		std::cout << "SDL bindings installed successfully." << std::endl;
	}


	uint32_t getWindowID(SDL_Window* window) {
		if (!window) {
			std::cout << "Game window not yet created." << std::endl;
			return 0;
		}
		if (!original_SDL_GetWindowID) {
			std::cout << "SDL_GetWindowID not available." << std::endl;
			return 0;
		}
		return original_SDL_GetWindowID(window);
	}
}