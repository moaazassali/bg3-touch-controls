#include <windows.h>
#include <functional>
#include <chrono>
#include <iostream>

#include "utils.h"


// We could use a separate timer thread with precise timings if needed
// but this should be good enough for our use case\
// Since our processing of touch input happens at a rate of 1/60s, we 
struct TapTimerContext {
	PTP_TIMER timer;
	std::function<void()> task;
};


static VOID CALLBACK TapThreadpoolTimerCallback(
	PTP_CALLBACK_INSTANCE      /*Instance*/,
	PVOID                      pv,
	PTP_TIMER                  pTimer)
{
	auto ctx = reinterpret_cast<TapTimerContext*>(pv);
	if (ctx && ctx->task) {
		try {
			ctx->task();
		}
		catch (...) {
			// swallow exceptions in timer callback to avoid terminating the process
		}
	}

	// Close and free the timer object.
	// CloseThreadpoolTimer will wait for outstanding callbacks; calling from the callback
	// is safe here for a one-shot timer because there are no other callbacks pending.
	if (pTimer) {
		CloseThreadpoolTimer(pTimer);
	}
	delete ctx;
}


void scheduleTask(std::function<void()> task, std::chrono::milliseconds delay)
{
	auto ctx = new (std::nothrow) TapTimerContext;
	if (!ctx) {
		std::cout << "Failed to allocate TapTimerContext for tap click." << std::endl;
		return;
	}

	ctx->timer = CreateThreadpoolTimer(TapThreadpoolTimerCallback, ctx, nullptr);
	ctx->task = std::move(task);
	if (!ctx->timer) {
		std::cout << "CreateThreadpoolTimer failed. Error: " << GetLastError() << std::endl;
		delete ctx;
		return;
	}

	// prepare a relative FILETIME: negative 100-ns units. 1ms = 1 * 10,000 * 100-ns
	LARGE_INTEGER liDue{};
	liDue.QuadPart = -static_cast<LONGLONG>(delay.count() * 10000LL);
	FILETIME ft{};
	ft.dwLowDateTime = liDue.LowPart;
	ft.dwHighDateTime = liDue.HighPart;

	// arm one-shot timer: period = 0, window = 0 (no coalescing window)
	SetThreadpoolTimer(ctx->timer, &ft, 0, 0);
}


SIZE getWindowSizePx(HWND hwnd)
{
	RECT rect;
	if (!GetClientRect(hwnd, &rect)) {
		return { 0, 0 };
	}
	return { rect.right - rect.left, rect.bottom - rect.top };
}