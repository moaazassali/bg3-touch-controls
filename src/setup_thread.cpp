#include <windows.h>
#include <cstdio>
#include <iostream>
#include <thread>
#include <chrono>
#include <InteractionContext.h>

#include "sdl_bindings/sdl_bindings.h"
#include "game_bindings.h"

LRESULT CALLBACK customWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void initializeInteractionContext(HWND hwnd);

WNDPROC g_originalWndProc = nullptr; // Store original window procedure
HINTERACTIONCONTEXT g_hInteractionContext;

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

/// <summary>
/// Subclass the game window to use our custom window procedure.
/// </summary>
/// <param name="hwnd">The handle to the game window.</param>
static void subclassGameWindow(HWND hwnd)
{
	g_originalWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)customWndProc);
	if (g_originalWndProc) {
		std::cout << "Window subclassed successfully." << std::endl;
	}
	else {
		std::cout << "FAILURE: Could not subclass window. Error: " << GetLastError() << std::endl;
	}
}

// Main start point for the dll injection thread
DWORD WINAPI setupThread(LPVOID lpParam)
{
	createConsole();
	bindToSDL();
	HWND hGameWnd = findMainWindowForCurrentProcess();
	subclassGameWindow(hGameWnd);
	initializeInteractionContext(hGameWnd);
	bindToGame();

	std::cout << "\n--- Press any key to close. ---" << std::endl;
	std::cin.get();
	return 0;
}