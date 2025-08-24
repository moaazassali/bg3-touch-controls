#include <windows.h>

// Our main dll injection thread
static DWORD WINAPI InjectionThread(LPVOID lpParam)
{
	return 0;
}

static BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		HANDLE hThread = CreateThread(nullptr, 0, InjectionThread, nullptr, 0, nullptr);
		if (hThread) {
			CloseHandle(hThread);
		}
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}