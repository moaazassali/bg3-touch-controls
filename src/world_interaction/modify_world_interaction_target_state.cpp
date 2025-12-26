#include <array>

#include "game_bindings.h"


using worldInteractionTargetResolver_t = void* (*)(void* a1);
worldInteractionTargetResolver_t original_worldInteractionTargetResolver = nullptr;

// disable by default
bool static shouldBypassWorldInteractionTargetResolver = true;
void* worldInteractionTargetResolver_a1 = nullptr;


static void* detour_worldInteractionTargetResolver(void* a1)
{
	worldInteractionTargetResolver_a1 = a1;
	if (shouldBypassWorldInteractionTargetResolver)
	{
		return a1;
	}
	return original_worldInteractionTargetResolver(a1);
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


// LLM generated
void resetWorldInteractionTargetResolver_a1() {
	if (!worldInteractionTargetResolver_a1) {
		return;
	}

	constexpr uint64_t NULL_HANDLE = 0xFFC0000000000000;
	uintptr_t a1 = (uintptr_t)worldInteractionTargetResolver_a1;

	// Clear 32-bit values
	*(uint32_t*)(a1 + 0x4C) = 0;   // 76
	*(uint32_t*)(a1 + 0x5C) = 0;   // 92
	*(uint32_t*)(a1 + 0x6C) = 0;   // 108
	*(uint32_t*)(a1 + 0x9C) = 0;   // 156
	*(uint32_t*)(a1 + 0x7C) = 0;   // 124
	*(uint32_t*)(a1 + 0x8C) = 0;   // 140

	// XMM operations - unpcklps xmm, xmm results in two packed 32-bit zeros (8 bytes total)
	uint64_t packedZeros = 0;

	// First set of XMM writes
	*(uint64_t*)(a1 + 0xB4) = packedZeros;  // 180 - movsd (8 bytes)
	*(uint32_t*)(a1 + 0xBC) = 0;            // 188 - movss (4 bytes)
	*(uint64_t*)(a1 + 0xA8) = 0;            // 168 - mov qword ptr
	*(uint32_t*)(a1 + 0xB0) = 0;            // 176

	// Second set of XMM writes
	*(uint64_t*)(a1 + 0xC0) = packedZeros;  // 192 - movsd (8 bytes)
	*(uint32_t*)(a1 + 0xC8) = 0;            // 200 - movss (4 bytes)

	// Third set of XMM writes
	*(uint64_t*)(a1 + 0xCC) = packedZeros;  // 204 - movsd (8 bytes)
	*(uint32_t*)(a1 + 0xD4) = 0;            // 212 - movss (4 bytes)

	// NULL_HANDLE assignments (r9 register)
	// These are the ID handles for different target types mentioned in README
	*(uint64_t*)(a1 + 0xD8) = NULL_HANDLE;  // 216 - only set for NPCs
	*(uint64_t*)(a1 + 0xE0) = NULL_HANDLE;  // 224 - only set for corpses
	*(uint64_t*)(a1 + 0xE8) = NULL_HANDLE;  // 232 - only set for items
	*(uint64_t*)(a1 + 0xF0) = NULL_HANDLE;  // 240 - any interactable object except corpses (generally the 'winner' target if interactable)
	*(uint64_t*)(a1 + 0xF8) = NULL_HANDLE;  // 248 - first hit: everything except sky (can be different than F0 if pressing on item through an invisible game object)

	// Clear flags
	*(uint8_t*)(a1 + 0x100) &= 0xFC;        // 256

	// Fourth set of XMM writes
	*(uint64_t*)(a1 + 0x114) = packedZeros; // 276 - movsd (8 bytes)
	*(uint32_t*)(a1 + 0x11C) = 0;           // 284 - movss (4 bytes)
	*(uint64_t*)(a1 + 0x108) = 0;           // 264 - mov qword ptr
	*(uint32_t*)(a1 + 0x110) = 0;           // 272

	// Fifth set of XMM writes
	*(uint64_t*)(a1 + 0x120) = packedZeros; // 288 - movsd (8 bytes)
	*(uint32_t*)(a1 + 0x128) = 0;           // 296 - movss (4 bytes)

	// Sixth set of XMM writes
	*(uint64_t*)(a1 + 0x12C) = packedZeros; // 300 - movsd (8 bytes)
	*(uint32_t*)(a1 + 0x134) = 0;           // 308 - movss (4 bytes)

	// Second batch of NULL_HANDLE assignments
	// Copy of the target type handles
	*(uint64_t*)(a1 + 0x138) = NULL_HANDLE; // 312
	*(uint64_t*)(a1 + 0x140) = NULL_HANDLE; // 320
	*(uint64_t*)(a1 + 0x148) = NULL_HANDLE; // 328
	*(uint64_t*)(a1 + 0x150) = NULL_HANDLE; // 336
	*(uint64_t*)(a1 + 0x158) = NULL_HANDLE; // 344

	// Clear flags
	// Seems to be flag for item or NPC type.
	// Either 0 for none, 1 for item, 2 for NPC
	*(uint8_t*)(a1 + 0x160) &= 0xFC;        // 352
}


std::array<uint64_t, 5> getCurrentWorldTargetEntityHandles()
{
	if (!worldInteractionTargetResolver_a1) {
		return { 0, 0, 0, 0, 0 };
	}

	uintptr_t a1 = (uintptr_t)worldInteractionTargetResolver_a1;
	return {
		*(uint64_t*)(a1 + 0xD8), // NPC
		*(uint64_t*)(a1 + 0xE0), // Corpse
		*(uint64_t*)(a1 + 0xE8), // Item
		*(uint64_t*)(a1 + 0xF0), // Interactable but not corpse
		*(uint64_t*)(a1 + 0xF8)  // First hit
	};
}