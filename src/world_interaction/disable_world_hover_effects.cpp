#include "game_bindings.h"


using worldInteractionTargetResolver_t = void* (*)(void* a1);
worldInteractionTargetResolver_t original_worldInteractionTargetResolver = nullptr;

// disable by default
bool static shouldBypassWorldInteractionTargetResolver = true;


static void* detour_worldInteractionTargetResolver(void* a1)
{
	if (shouldBypassWorldInteractionTargetResolver)
	{
		return a1;
	}
	else return original_worldInteractionTargetResolver(a1);
}


bool installWorldInteractionTargetResolverHook(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_worldInteractionTargetResolver = "48 8B C4 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 0F 29 70 ?? 0F 29 78 ?? 44 0F 29 40 ?? 44 0F 29 88 ?? ?? ?? ?? 44 0F 29 90 ?? ?? ?? ?? 4C 8B E9";
	uintptr_t matchAddr = findPattern(pattern_worldInteractionTargetResolver, start, end);
	return installHook((void*)matchAddr, &detour_worldInteractionTargetResolver, (void**)&original_worldInteractionTargetResolver, "worldInteractionTargetResolverHook");
}


void bypassWorldInteractionTargetResolver()
{
	shouldBypassWorldInteractionTargetResolver = true;
}


void useOriginalWorldInteractionTargetResolver()
{
	shouldBypassWorldInteractionTargetResolver = false;
}