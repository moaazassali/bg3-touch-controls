#pragma once
#include <SDL.h>


void bindToSDL();

bool installSDLCreateWindowHook();

namespace sdl_bindings {
	using SDL_CreateWindow_t = SDL_Window* (*)(const char* title, int x, int y, int w, int h, Uint32 flags);
}