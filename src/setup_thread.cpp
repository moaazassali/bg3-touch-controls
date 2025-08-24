#include <windows.h>
#include <cstdio>
#include <iostream>

static void createConsole()
{
	AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONIN$", "r", stdin);

	std::cout << "DLL Injected" << std::endl;
}

// Main start point for the dll injection thread
DWORD WINAPI setupThread(LPVOID lpParam)
{
    createConsole();

    std::cout << "\n--- Press Enter to close. ---" << std::endl;
    std::cin.get();
	return 0;
}