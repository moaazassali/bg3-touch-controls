#pragma once
#include <atomic>

enum class InertiaState {
	NotInInertia,
	// The first frame of inertia after user input ends
	// We need to handle this frame differently to set the initial velocity of inertia
	InertiaStart,
	InInertia,
};

struct PendingCameraUpdates {
	std::atomic<float> angleDelta{ 0.0f };
	std::atomic<float> zoomDelta{ 0.0f };
	std::atomic<float> xPanDelta{ 0.0f };
	std::atomic<float> yPanDelta{ 0.0f };
	std::atomic<float> pitch{ 34.5f }; // make initial value configurable
	std::atomic<InertiaState> inertiaState{ InertiaState::NotInInertia };
};

extern PendingCameraUpdates g_pendingCameraUpdates;