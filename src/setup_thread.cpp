#include <windows.h>
#include <cstdio>
#include <iostream>
#include <thread>
#include <chrono>

static void createConsole()
{
	AllocConsole();
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONIN$", "r", stdin);

	std::cout << "DLL Injected" << std::endl;
}

BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam)
{
	DWORD windowPid = 0;
	GetWindowThreadProcessId(hwnd, &windowPid);

	// Only match visible windows belonging to our process
	if (windowPid == GetCurrentProcessId() && IsWindowVisible(hwnd)) {
		char className[256] = { 0 };
		GetClassNameA(hwnd, className, sizeof(className));
		// (Temporary) Exclude console window class
		if (strcmp(className, "ConsoleWindowClass") != 0) {
			*reinterpret_cast<HWND*>(lParam) = hwnd;
			std::cout << "Found game window with class name: " << className << std::endl;
			return FALSE; // Found game window, stop
		}
	}
	return TRUE;
}

static HWND findMainWindowForCurrentProcess()
{
	std::cout << "Waiting for game window to launch..." << std::endl;
	HWND hwnd = nullptr;
	while (!hwnd) {
		EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&hwnd));;
		if (!hwnd) {
			std::this_thread::sleep_for(std::chrono::milliseconds(250));
		}
	}
	return hwnd;
}

// Main start point for the dll injection thread
DWORD WINAPI setupThread(LPVOID lpParam)
{
	createConsole();
	HWND hGameWnd = findMainWindowForCurrentProcess();

	std::cout << "\n--- Press any key to close. ---" << std::endl;
	std::cin.get();
	return 0;
}