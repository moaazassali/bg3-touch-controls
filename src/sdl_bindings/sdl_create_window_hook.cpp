#include <iostream>
#include <SDL.h>
#include <MinHook.h>

#include "sdl_bindings/sdl_bindings.h"


sdl_bindings::SDL_CreateWindow_t original_SDL_CreateWindow = nullptr;


static SDL_Window* detour_SDL_CreateWindow(const char* title, int x, int y, int w, int h, Uint32 flags) {
	SDL_Window* window = original_SDL_CreateWindow(title, x, y, w, h, flags);
	if (!window) {
		std::cout << "SDL_CreateWindow failed." << std::endl;
		return nullptr;
	}
	std::cout << "SDL_CreateWindow called successfully. Window title: " << title << std::endl;
	return window;
}


bool installSDLCreateWindowHook() {
	void* pTarget = nullptr;
	if (MH_CreateHookApiEx(L"SDL2.dll", "SDL_CreateWindow", &detour_SDL_CreateWindow, (void**)&original_SDL_CreateWindow, &pTarget) != MH_OK) {
		std::cout << "Failed to create SDL_CreateWindow hook." << std::endl;
		return false;
	}
	if (MH_EnableHook(pTarget) != MH_OK)
	{
		std::cout << "Failed to enable SDL_CreateWindow hook" << std::endl;
		return false;
	}
	std::cout << "SDL_CreateWindow hook installed successfully." << std::endl;
	return true;
}