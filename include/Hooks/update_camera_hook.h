#pragma once

// Adapted from BG3_NativeCameraTweaks:
// https://github.com/ersh1/BG3_NativeCameraTweaks/blob/876f2da98ede96d3db2884ffd9fb25687ada5ff1/src/Hooks.h
using updateCamera_t = void(*)(uint64_t a1, uint64_t a2, RE::DeltaTimeObject* a3, RE::UnkObject* a4);

bool installUpdateCameraHook(uintptr_t start, uintptr_t end);