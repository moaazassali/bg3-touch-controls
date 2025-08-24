#include <windows.h>
#include <cstdio>
#include <iostream>
#include <thread>
#include <chrono>
#include <InteractionContext.h>

LRESULT CALLBACK customWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

static void CALLBACK interactionContextCallback(void* clientData, const INTERACTION_CONTEXT_OUTPUT2* output) {

}

static void initializeInteractionContext(HWND hwnd)
{
	if (g_hInteractionContext == nullptr) {
		auto hResult = CreateInteractionContext(&g_hInteractionContext);
		if FAILED(hResult) {
			std::cout
				<< "FAILURE: Could not create Interaction Context. HRESULT: "
				<< std::hex << hResult << std::dec << std::endl;
			return;
		}
		std::cout << "Interaction Context created." << std::endl;

		// https://learn.microsoft.com/en-us/windows/win32/api/interactioncontext/ne-interactioncontext-interaction_id
		// https://learn.microsoft.com/en-us/windows/win32/api/interactioncontext/ne-interactioncontext-interaction_configuration_flags
		INTERACTION_CONTEXT_CONFIGURATION config[] = {
			{
				INTERACTION_ID_TAP,
				INTERACTION_CONFIGURATION_FLAG_TAP |
				INTERACTION_CONFIGURATION_FLAG_TAP_DOUBLE |
				INTERACTION_CONFIGURATION_FLAG_TAP_MULTIPLE_FINGER
			},
			{
				INTERACTION_ID_HOLD,
				INTERACTION_CONFIGURATION_FLAG_HOLD |
				INTERACTION_CONFIGURATION_FLAG_HOLD_MULTIPLE_FINGER
			}
		};

		hResult = SetInteractionConfigurationInteractionContext(
			g_hInteractionContext,
			2,
			config
		);
		if FAILED(hResult) {
			std::cout
				<< "FAILURE: Could not set Interaction Context configuration. HRESULT: "
				<< std::hex << hResult << std::dec << std::endl;
		}
		std::cout << "Interaction Context configuration set." << std::endl;


		// Register our callback function to receive the events from the Interaction Context
		hResult = RegisterOutputCallbackInteractionContext2(g_hInteractionContext, interactionContextCallback, nullptr);
		if FAILED(hResult) {
			std::cout 
				<< "FAILURE: Could not register interaction context output callback. HRESULT: "
				<< std::hex << hResult << std::dec << std::endl;
		}
		std::cout << "Interaction Context callback registered." << std::endl;
	}
}

// Main start point for the dll injection thread
DWORD WINAPI setupThread(LPVOID lpParam)
{
	createConsole();
	HWND hGameWnd = findMainWindowForCurrentProcess();
	subclassGameWindow(hGameWnd);
	initializeInteractionContext(hGameWnd);

	std::cout << "\n--- Press any key to close. ---" << std::endl;
	std::cin.get();
	return 0;
}