#include <Windows.h>
#include "MinHook.h"
#include <stdio.h>

typedef int (WINAPI *MESSAGEBOXW)(HWND, LPCWSTR, LPCWSTR, UINT);
typedef DWORD(WINAPI* PFPowerSettingRegisterNotification)(__in LPCGUID SettingGuid, __in DWORD Flags, __in HANDLE Recipient, __out PVOID* RegistrationHandle);

// Pointer for calling original MessageBoxW.
MESSAGEBOXW fpMessageBoxW = NULL;

PFPowerSettingRegisterNotification fpPowerSettingRegisterNotification = NULL;

// Detour function which overrides MessageBoxW.
int WINAPI DetourMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
	return fpMessageBoxW(hWnd, L"Hooked!", lpCaption, uType);
}

DWORD WINAPI _OnPowerSettingRegisterNotification(
	__in LPCGUID SettingGuid,
	__in DWORD Flags,
	__in HANDLE Recipient,
	__out PVOID* RegistrationHandle
	)
{
	*(ULONG_PTR*)RegistrationHandle = 0xdeadbeef;
	printf("Called PowerSettingRegisterNotification\n");
	return ERROR_SUCCESS;
}

int main()
{
	HMODULE hmodule = NULL;
	do
	{
		// Initialize MinHook.
		if (MH_Initialize() != MH_OK)
		{
			break;
		}

		// Initialize Extension feature.
		if (MH_InitializeEx() != MH_OK)
		{
			break;
		}

		// Create a hook for MessageBoxW, in disabled state.
		//if (MH_CreateHook(&MessageBoxW, &DetourMessageBoxW,
		//	reinterpret_cast<LPVOID*>(&fpMessageBoxW)) != MH_OK)
		//{
		//	break;
		//}

		// or you can use the new helper funtion like this.
		if (MH_CreateHookApiEx(
			L"user32.dll", "MessageBoxW", &DetourMessageBoxW, reinterpret_cast<LPVOID*>(&fpMessageBoxW)) != MH_OK)
		{
			break;
		}

		hmodule = ::GetModuleHandleW(L"powrprof.dll");
		if (NULL != hmodule)
		{
			printf("powrprof.dll already loaded.\n");
			::FreeLibrary(hmodule);
		}
		else
			printf("powrprof.dll does not load yet.\n");
			

		// or you can use the new helper funtion like this.
		MH_STATUS status = MH_CreateHookApiEx(
			L"powrprof.dll", "PowerSettingRegisterNotification", &_OnPowerSettingRegisterNotification, reinterpret_cast<LPVOID*>(&fpPowerSettingRegisterNotification));
		if (status != MH_OK)
		{
			break;
		}

		// Enable the hook for MessageBoxW.
		if (MH_EnableHook(&MessageBoxW) != MH_OK)
		{
			break;
		}

		hmodule = ::LoadLibraryW(L"powrprof.dll");
		if (NULL != fpPowerSettingRegisterNotification)
		{
			printf("PowerSettingRegisterNotification hooked!!\n");

			PFPowerSettingRegisterNotification PowerSettingRegisterNotification = (PFPowerSettingRegisterNotification)::GetProcAddress(hmodule, "PowerSettingRegisterNotification");
			HANDLE handle = NULL;
			PowerSettingRegisterNotification(NULL, 0, NULL, &handle);
			printf("PowerSettingRegisterNotification returns 0x%p", handle);
		}

		// Expected to tell "Hooked!".
		::MessageBoxW(NULL, L"Not hooked...", L"MinHook Sample", MB_OK);

		// Disable the hook for MessageBoxW.
		if (MH_DisableHook(&MessageBoxW) != MH_OK)
		{
			break;
		}

		// Expected to tell "Not hooked...".
		::MessageBoxW(NULL, L"Not hooked...", L"MinHook Sample", MB_OK);

		// Uninitialize MinHook.
		if (MH_Uninitialize() != MH_OK)
		{
			break;
		}


	} while (false);

	if (NULL != hmodule)
		::FreeLibrary(hmodule);

	return 0;
}