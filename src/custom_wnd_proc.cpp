#include <windows.h>
#include <iostream>

extern WNDPROC g_originalWndProc;

LRESULT CALLBACK customWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	switch (msg) {
	case WM_TOUCH:
		// Handle touch input here
		std::cout << "Touch input received!" << std::endl;
		return 0; // blocks WM_TOUCH from reaching the original window procedure
	}
	// Call the original window procedure for all other messages
	return CallWindowProc(g_originalWndProc, hwnd, msg, wParam, lParam);
}