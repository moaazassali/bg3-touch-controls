#pragma once
#include <cstdint>


// This struct holds the internal cursor position used by the game.
// This does not necessarily reflect the actual OS cursor position.
// If mouse input is used, then it should always be the same. However, if only SDL mouse events
// are used without actually moving the OS cursor, then this position will not be the same as the
// OS cursor position, which is the case when using touch controls with this mod since we only send
// SDL mouse events (although we could also move the OS cursor if needed).
// There is another struct in-game that holds cursor position as well several bytes after the
// g_isCursorOverUI boolean.
// It seems to be updated after processing input, while this one is updated immediately as soon
// as the mouse move event is received from SDL_PollEvent() by the game.
namespace reverse {
	struct InternalCursorPosition {
		float x;
		float y;
	};

	constexpr const uintptr_t offset_internalCursorPosition = 0x611BAE0;

	extern InternalCursorPosition** g_internalCursorPosition;
}