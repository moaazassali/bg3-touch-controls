#include <iostream>
#include <MinHook.h>

#include "sdl_bindings/sdl_bindings.h"


void bindToSDL()
{
	if (MH_Initialize() != MH_OK) {
		std::cout << "Failed to initialize MinHook." << std::endl;
		return;
	}
	if (!installSDLCreateWindowHook()) {
		MH_Uninitialize();
		return;
	}
	std::cout << "SDL bindings installed successfully." << std::endl;
}