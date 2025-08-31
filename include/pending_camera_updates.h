#pragma once
#include <atomic>

struct PendingCameraUpdates {
	std::atomic<float> angleDelta{ 0.0f };
	std::atomic<float> zoomDelta{ 0.0f };
	std::atomic<float> xPanDelta{ false };
	std::atomic<float> yPanDelta{ false };
};

extern PendingCameraUpdates g_pendingCameraUpdates;