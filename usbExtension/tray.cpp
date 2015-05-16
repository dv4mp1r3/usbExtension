#include "tray.h"

BOOL TrayMessage (HWND hWnd, DWORD dwMessage, UINT uID, HICON hIcon, PWSTR pszTip)
{
	NOTIFYICONDATA ntficon; 
	ntficon.cbSize = sizeof(NOTIFYICONDATA); 
	ntficon.hWnd = hWnd; 
	ntficon.uID = uID; 
	ntficon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP; 
	ntficon.uCallbackMessage = MYWM_NOTIFYICON;
	ntficon.hIcon = hIcon; 
	
	if (pszTip)
		lstrcpyn(ntficon.szTip, pszTip, 64);
	else
		ntficon.szTip[0] = 0;

	return Shell_NotifyIcon(dwMessage, &ntficon); 
}