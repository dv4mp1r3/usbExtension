#pragma once
#include <Windows.h>

#define MYWM_NOTIFYICON (WM_USER + 1)

static wchar_t TRAY_MAIN_STRING[] = L"USB Extension 1.0.2";


BOOL TrayMessage (HWND hWnd, DWORD dwMessage, UINT uID, HICON hIcon, PWSTR pszTip);

