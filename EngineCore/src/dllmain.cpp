#if defined(_MSC_VER)
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>

// NOLINTNEXTLINE
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}

#endif
