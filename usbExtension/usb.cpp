#include "usb.h"

DWORD USB_DEVICE::createShortcut()
{
	HRESULT hres; 
    IShellLink* psl; 
 
	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl); 
    if (SUCCEEDED(hres)) 
    { 
        IPersistFile* ppf; 
 
		psl->SetPath(strLabel); 
        psl->SetDescription(SHORTCUT_DESCRIPTION); 
 
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf); 
 
        if (SUCCEEDED(hres))
		{
			
			lstrcat(strShortcutName, L"Removeable disk (");
			lstrcat(strShortcutName, strLabel);
			strShortcutName[lstrlen(strShortcutName) -2] = 0;
			lstrcat(strShortcutName, L")");
			lstrcat(strShortcutName, L".lnk");
			hres = ppf->Save(strShortcutName, TRUE);           
            ppf->Release();
        } 
        psl->Release(); 


    } 
    
    return GetLastError();
}

USB_DEVICE::USB_DEVICE(ULONG label)
{

	ZeroMemory(strShortcutName, sizeof(strShortcutName));
	if (label)
	{
		this->label = label;
		setLabel(label);
		createShortcut();
	}
	
}

void USB_DEVICE::setLabel( ULONG unitmask )
{
	char i;
	char b[2];

	lstrcpy(strLabel,L"A:\\");

	for (i = 0; i < 26; ++i)
	{
		if (unitmask & 0x1)
			break;
		unitmask = unitmask >> 1;
	}

	i +='A';
	b[0] = i; b[1] = 0;

	memcpy(strLabel,b,2);
	//i = 0;
}

void USB_DEVICE::setLabel(const wchar_t* label )
{
	lstrcpyn(strLabel, label, sizeof(strLabel));
}

bool USB_DEVICE::equal(USB_DEVICE const &tmp)
{
	return (label == tmp.label);
}

bool USB_DEVICE::operator==(USB_DEVICE const &tmp)
{
	return equal(tmp);
}

bool USB_DEVICE::equal(ULONG unitmask)
{
	return (unitmask == label);
}

bool USB_DEVICE::equal(wchar_t* const label)
{
	if (!lstrcmpi(strLabel, label))
		return true;
	return false;

}

bool USB_DEVICE::deleteShortcut(const wchar_t* path)
{
	SHChangeNotify(SHCNE_DELETE, SHCNF_PATH | SHCNF_FLUSHNOWAIT, reinterpret_cast<LPCVOID>(path), NULL);
	return DeleteFile(path);		
}
