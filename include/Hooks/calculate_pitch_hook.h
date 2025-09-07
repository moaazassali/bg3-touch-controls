#pragma once

// Adapted from BG3_NativeCameraTweaks:
// https://github.com/ersh1/BG3_NativeCameraTweaks/blob/876f2da98ede96d3db2884ffd9fb25687ada5ff1/src/Hooks.h
using calculatePitch_t = float(*)(RE::CameraObject* a1, uint8_t a2, uint8_t a3);

bool installCalculatePitchHook(uintptr_t start, uintptr_t end);