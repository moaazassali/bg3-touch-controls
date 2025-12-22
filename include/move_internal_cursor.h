#pragma once
#include <cstdint>


bool installMoveInternalCursorModule(uintptr_t start, uintptr_t end);
bool moveInternalCursor(float x, float y);
bool isCursorOverUI();