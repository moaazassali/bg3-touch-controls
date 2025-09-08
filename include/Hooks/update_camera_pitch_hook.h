#pragma once
#include <cstdint>

#include "RE/Camera.h"


// Adapted from BG3_NativeCameraTweaks:
// https://github.com/ersh1/BG3_NativeCameraTweaks/blob/876f2da98ede96d3db2884ffd9fb25687ada5ff1/src/Hooks.h
using updateCameraPitch_t = void(*)(uint64_t a1, uint64_t a2, RE::CameraObject* a3, RE::DeltaTimeObject* a4);
using getCameraDefinition_t = RE::CameraDefinition* (*)(RE::CameraObject* a1);

bool installUpdateCameraPitchHook(uintptr_t start, uintptr_t end);