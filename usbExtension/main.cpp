
#include "usb.h"
#include "inifile.h"
#include "tray.h"
#include "resource.h"
#include <vector>
#include <windows.h>

// WM_USER 1024
#define WM_DEVICE_FORMAT 2000
#define WM_DEVICE_OPEN   3000
#define WM_DEVICE_UNJECT 4000

using std::wstring;

std::vector<USB_DEVICE*> devices;
CIniFile settings;
HANDLE hMutex;
HMENU hMainMenu = NULL;

bool detectUSB, detectCD, detectNETWORK;

LRESULT CALLBACK _WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/*
	Поиск в векторе devices устройство с меткой label
	Входящие параметры:
	label - метка для поиска

	Возвращаемое значение:
	Индекс устройства в векторе, либо -1, если не удалось найти совпадение
*/
int searchDeviceByLabel(wchar_t* const label);

/*
	Добавление ярлыков к устройствам, которые были подключены ранее запуска программы

	Возвращаемое значение:
	Количество добавленных ярлыков
*/
int checkConnectedDevices();

/*
	Проверка мьютекса. Необходима для запуска одной копии программы

	Возвращаемое значение:
	true при первом запуске программы
*/
bool checkMutex();

/*
	Удаление ярлыков с меткой SHORTCUT_DESCRIPTION из текущей папки

	Возвращаемое значение:
	Количество удаленных ярлыков
*/
int deleteOldShortcuts();

/*
	Регистрация уведомлений о подключении новых устройств и монтировании файловой системы
	Входящие параметры:
	hWnd - дескриптор окна, которое будет получать уведомления

	Важно:
	Уведомления обрабатываются сообщением WM_DEVICECHANGE

	Возвращаемое значение:
	Результат попытки регистрации уведомлений
*/
void recreateMenu(HWND hwnd)
{
	if (hMainMenu)
	{
		TrayMessage(hwnd, NIM_DELETE, 0, 0, 0);	
		DestroyMenu(hMainMenu);
	}
	hMainMenu = CreatePopupMenu();
	AppendMenu( hMainMenu, MF_STRING, 1002, L"Delete invalid shortcuts");
	//AppendMenu( hMainMenu, MF_STRING, 1003, L"About");
	AppendMenu( hMainMenu, MF_SEPARATOR, 0, L"");
	AppendMenu( hMainMenu, MF_STRING, 1004, L"Exit");

	wstring menuFormat, menuUnject, menuOpen;
	//2001 - 2(operation), 001(device index in devices list)

	for (int i = 0; i< devices.size(); i++)
	{
		menuOpen     = L"Open in Explorer " + "E:\\";
		menuUnject   = L"Unject " + "E:\\"; 
		menuFormat   = L"Format " + "E:\\"; 
		AppendMenu( hMainMenu, MF_SEPARATOR, 0, L"");
		AppendMenu( hMainMenu, MF_STRING, 1004, menuOpen.c_str() );
		AppendMenu( hMainMenu, MF_STRING, 1004, menuFormat.c_str() );
		AppendMenu( hMainMenu, MF_STRING, 1004, menuUnject.c_str() );
	}
}

HDEVNOTIFY USBRegister(HWND hWnd)
{
	DEV_BROADCAST_DEVICEINTERFACE dbi = { sizeof(dbi), DBT_DEVTYP_DEVICEINTERFACE };
	CLSIDFromString(const_cast<LPOLESTR>(L"{53f56307-b6bf-11d0-94f2-00a0c91efb8b}"), &dbi.dbcc_classguid);
	
        HDEVNOTIFY hdefwew = RegisterDeviceNotification(reinterpret_cast<HANDLE>(hWnd), 
                                                       reinterpret_cast<LPVOID>(&dbi), 
                                                        DEVICE_NOTIFY_WINDOW_HANDLE);
        
        return 0;
}

bool getParam(const wstring& section, const wstring& param)
{
	wstring res = settings.GetKeyValue(section, param);
	if (res == L"True")
                return true;
	return false;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)

{
	
	CoInitialize(NULL);
	wchar_t strShortcut[MAX_PATH];

	wchar_t configPath[MAX_PATH];
	int cpLen = GetModuleFileName(0, configPath, MAX_PATH);
	if (!cpLen)
		return MessageBox(0, L"GetModuleFileName error.", L"System error", MB_OK|MB_ICONERROR);

	while(cpLen)
	{
		if (configPath[cpLen] == L'\\')
		{
			lstrcat(configPath, L"config.ini");
			break;
		}
		configPath[cpLen] = 0;
		cpLen--;
	}

	const wstring IniSectionOptions = L"show_options";
	if (!settings.Load(configPath, true)) 
		return MessageBox(0, L"Can't to load config.ini.", L"INI error", MB_OK|MB_ICONERROR);
	if (!checkMutex()) 
		return MessageBox(0, L"Program can only be run in a single copy.", L"Mutex error", MB_OK|MB_ICONERROR);

	if (SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY | CSIDL_FLAG_CREATE, NULL, 
						SHGFP_TYPE_CURRENT, strShortcut) != S_OK 
						||
						!SetCurrentDirectory(strShortcut))
		return 0;
	
	deleteOldShortcuts();
	
	detectCD      = getParam(IniSectionOptions, L"cd_rom");
	detectUSB     = getParam(IniSectionOptions, L"usb");
	detectNETWORK = getParam(IniSectionOptions, L"network_drives");

	if (settings.GetKeyValue(L"show_method", L"always") == L"True")
		checkConnectedDevices();

	HWND hWnd;
	MSG Message;
	WNDCLASSEX wcex;

	ZeroMemory(&wcex, sizeof(wcex)); 

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style            = 0;
    wcex.lpfnWndProc      = _WndProc;
    wcex.cbClsExtra       = 0;
    wcex.cbWndExtra       = 0;
    wcex.hInstance        = hInstance;
    wcex.hIcon            = NULL;
    wcex.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName     = NULL;
    wcex.lpszClassName    = L"FRM";
    wcex.hIconSm          = NULL;

	if (RegisterClassEx(&wcex))
	{
		hWnd = CreateWindowEx(WS_EX_WINDOWEDGE|WS_EX_APPWINDOW, 
								L"FRM", L"", 
								WS_SYSMENU|WS_MINIMIZEBOX, 
								200, 200, 0, 0, 
								NULL, NULL, hInstance, NULL);
		if (hWnd)
		{
			ShowWindow(hWnd, SW_HIDE);
			UpdateWindow(hWnd);
			
			USBRegister(hWnd);

			if (settings.GetKeyValue(L"other", L"tray_icon") == L"True")
            {
				HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
                //HICON icon = reinterpret_cast<HICON>(LoadImage(hInstance, NULL, IMAGE_ICON, 0, 0, LR_LOADFROMFILE));
                TrayMessage(hWnd, NIM_ADD, 0, hIcon, TRAY_MAIN_STRING);
            }
			while (GetMessage(&Message, 0, 0, 0))
			{
				TranslateMessage(&Message);
				DispatchMessage(&Message);
			}
		}
			
	}
 
    return Message.wParam;
}

LRESULT CALLBACK _WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
	PDEV_BROADCAST_VOLUME pDevVolume = (PDEV_BROADCAST_VOLUME)pHdr;

	switch (message)
	{		
	case WM_DEVICECHANGE:
		 if (DBT_DEVICEARRIVAL == wParam) 
		 {
			int usbEquals = 0;
			switch( pHdr->dbch_devicetype ) 
			{
				case DBT_DEVTYP_DEVICEINTERFACE:
					break;

				case DBT_DEVTYP_VOLUME:
					usbEquals = 0;
					if ((pDevVolume->dbcv_flags & DBTF_MEDIA && detectCD) || 
						(pDevVolume->dbcv_flags & DBTF_NET && detectNETWORK) ||
						(pDevVolume->dbcv_flags == 0 && detectUSB))
					{
						for (int i = 0; i < devices.size(); i++)
						if (!devices[i]->equal((ULONG)pDevVolume->dbcv_unitmask))
							usbEquals++;

						if (usbEquals == devices.size())
							devices.push_back(new USB_DEVICE((ULONG)pDevVolume->dbcv_unitmask));

						recreateMenu(hWnd);
					}
					
						
					break;
			}
		}
		else if (DBT_DEVICEREMOVECOMPLETE == wParam && pHdr->dbch_devicetype == DBT_DEVTYP_VOLUME)
		{
			for (int i = 0; i < devices.size(); i++)
				if (devices[i]->equal(pDevVolume->dbcv_unitmask)) 
				{					
					delete devices[i];
					devices.erase(devices.begin() + i);
					return 0; 
				}
			deleteOldShortcuts();
			recreateMenu(hWnd);
		}
		return 0;
	case WM_CLOSE: 
		ReleaseMutex(hMutex);
		TrayMessage(hWnd, NIM_DELETE, 0, 0, 0);	
		DestroyMenu(hMainMenu);
		while (devices.size())
		{			
			delete devices[0];
			devices.erase(devices.begin());
		}
		PostQuitMessage(0);
	case MYWM_NOTIFYICON:
		if (lParam == WM_RBUTTONUP)
		{
			POINT cur_pos;
			if (GetCursorPos(&cur_pos)) 
			{ 
				SetForegroundWindow(hWnd);
				TrackPopupMenuEx(hMainMenu,TPM_HORIZONTAL|TPM_LEFTALIGN,cur_pos.x,cur_pos.y,hWnd,NULL);   
			}
			
		}		
		return 0;
	case WM_CREATE:
		recreateMenu(hWnd);
		SetForegroundWindow(hWnd);
		return 0;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case 1002:
			deleteOldShortcuts();
			break;
		case 1003:
			//MessageBox(0, L"About", L"", MB_OK|MB_ICONINFORMATION);
			break;
		case 1004:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		}
		return 0;
	 default:
            return DefWindowProc(hWnd, message, wParam, lParam);	

	}

	return 0;
}

int searchDeviceByLabel(wchar_t* const label)
{
	for (int i = 0; i < devices.size(); i++)
	{
		if (devices[i]->equal(label))
		{
			return i;
		}
	}

	return -1;
}

int deleteOldShortcuts()
{
	int count = 0;
	WIN32_FIND_DATA ffd;

	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(L"*.lnk", &ffd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		IShellLink* psl;
		HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*) &psl);
		wchar_t* tempStr = new wchar_t[MAX_PATH];

		if (tempStr)
		{
			do
			{				
				if (SUCCEEDED(hr))
				{
					IPersistFile* ppf = NULL;
					hr = psl->QueryInterface( IID_IPersistFile, (LPVOID *) &ppf);
					if (SUCCEEDED(hr))
					{
						hr = ppf->Load(ffd.cFileName, STGM_READ);

						if (SUCCEEDED(hr))
						{
							//WIN32_FIND_DATA wfd;
							psl->GetPath(tempStr, MAX_PATH, NULL, SLGP_UNCPRIORITY | SLGP_RAWPATH);

							UINT destType = GetDriveType(tempStr);
							psl->GetDescription(tempStr, MAX_PATH);
						
							if (wcsstr(tempStr, SHORTCUT_DESCRIPTION) 
								&& lstrlen(tempStr) == lstrlen(SHORTCUT_DESCRIPTION) 
								&& destType == DRIVE_NO_ROOT_DIR)
								if (USB_DEVICE::deleteShortcut(ffd.cFileName)) 
									count++;
							ppf->Release();
						}
						
					}
				}
			
			}while(FindNextFile(hFind, &ffd));
			delete tempStr;
			psl->Release();
		}
		FindClose(hFind);
	}
	
	return count;
}

bool checkMutex()
{
	const wchar_t mutex[] = L"367etefjkghsdf43kj6erit80p6u234yh23-0reujswd";
	hMutex = CreateMutex(NULL,true, mutex);

	if (hMutex != reinterpret_cast<HANDLE>(ERROR_ALREADY_EXISTS) && GetLastError() == 0)
		return true;
	return false;
}

int checkConnectedDevices()
{
	int result = 0;
	wchar_t label[] = L"A:\\";
	for (int i = 0; i < 25; i++)
	{
		label[0]++;
		UINT deviceType = GetDriveType(label);
		if ((deviceType == DRIVE_REMOVABLE && detectUSB) ||
			(deviceType == DRIVE_CDROM && detectCD) ||
			(deviceType == DRIVE_REMOTE && detectNETWORK))
		{
			devices.push_back(new USB_DEVICE(0));
			int devIndex = devices.size() - 1;
			devices[devIndex]->setLabel(label);
			devices[devIndex]->createShortcut();

			result++;

		}
	}
	return result;
}
