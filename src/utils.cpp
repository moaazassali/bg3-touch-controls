#include <windows.h>
#include <functional>
#include <chrono>
#include <iostream>

#include "utils.h"
#include "reverse/action_request.h"


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


void printModuleInstallHeader(const char* moduleName)
{
	std::cout << "===== Installing " << moduleName << " Module =====" << std::endl;
}


void printModuleInstallResult(bool success)
{
	if (success) {
		std::cout << "Success :)" << std::endl;
	}
	else {
		std::cout << "FAILED" << std::endl;
	}
}


void printPatternNotFoundError(const char* pattern, const char* patternName)
{
	std::cout << "Failed to find pattern " << pattern << " for " << patternName << "." << std::endl;
}


void printPatternFoundMessage(uintptr_t address, const char* patternName)
{
	std::cout << "Found pattern for " << patternName << " at address: 0x" << std::hex << address << std::dec << std::endl;
}


void printActionRequestDetails(reverse::ActionRequest* a2) {
	if (a2 && a2->action) {
		std::string actionName;
		if (a2->action->name && a2->action->nameLen > 0) {
			actionName.assign(a2->action->name, static_cast<size_t>(a2->action->nameLen));
		}
		std::cout
			<< (actionName.empty() ? "null" : actionName)
			<< ", ID: " << a2->actionId
			<< ", Device: " << a2->deviceId
			<< ", State: " << static_cast<int>(a2->actionState)
			<< ", Scaled: " << a2->currentPressValue
			<< ", Raw: " << a2->currentRawValue
			<< ", Pressed: " << static_cast<int>(a2->currentIsPressed)
			<< ", Prev press: " << a2->prevPressValue
			<< ", Prev raw: " << a2->prevRawValue
			<< ", Prev pressed: " << static_cast<int>(a2->prevIsPressed)
			<< ", DeviceDup: " << a2->deviceId_dup1
			<< ", InputId: " << a2->inputId
			<< ", PlayerIdx: " << static_cast<int>(a2->playerIndex)
			<< ", PlayerIdxDup: " << static_cast<int>(a2->playerIndex_dup1)
			<< std::endl;
	}
}