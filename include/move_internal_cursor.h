#pragma once
#include <cstdint>
#include <functional>


bool installMoveInternalCursorModule(uintptr_t start, uintptr_t end);
bool moveInternalCursor(float x, float y);
bool isCursorOverUI();
void callAfterGameProcessesCursorUpdate(std::function<void()> func, uint8_t frameDelay = 2);