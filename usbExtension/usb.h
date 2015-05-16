#pragma once

#include <Windows.h>
#include <dbt.h>
#include <shlobj.h>

static const wchar_t SHORTCUT_DESCRIPTION[] = L"Storage Device";

class USB_DEVICE
{
	ULONG label;
	
	// Буфер для хранения корня диска подключенного устройства (C:\\, D:\\ и т.д.)
	wchar_t strLabel[7];

	// Путь к ярлыку, который указывает на корень диска
	wchar_t strShortcutName[100];
public:
    /*
	Создание ярлыка к логическому диску устройства в текущей папке

	Возвращаемое значение:
	Код ошибки при создании ярлыка. 0 - при удачном создании ярлыка
	*/
	DWORD createShortcut();
	/*
		Задание метки логического диска для устройства

		Входящие параметры:
		unitmask - метка логического диска. Получается при монтировании ФС, сразу после подключения устройства.
		Для получения необходимо обработать сообщение WM_DEVICECHANGE -> DBT_DEVICEARRIVAL -> DBT_DEVTYP_VOLUME.
		И передать значение PDEV_BROADCAST_VOLUME.dbcv_unitmask

	*/
	void setLabel(ULONG unitmask );
	void setLabel(const wchar_t* label );

	/*
		Конструктор

		Входящие параметры:
		ULONG label - метка устройства. 

	*/
	USB_DEVICE(ULONG label);

	/*
		Сравнивает устройства

		Входящие параметры:
		tmp - с чем сравнивается

		Возвращаемое значение: true, если экземпляры класса описывают одно устройство
		Сравнение идет между именами устройств

	*/
	bool equal(USB_DEVICE const &tmp);

	/*
		Сравнивает устройства

		Входящие параметры:
		label - метка устройства
		Для получения имени необходимо обработать сообщение WM_DEVICECHANGE -> DBT_DEVICEARRIVAL -> DBT_DEVTYP_VOLUME

		Возвращаемое значение: true, если экземпляры класса описывают одно устройство

	*/
	bool equal(ULONG unitmask);
	bool equal(wchar_t* const label);

	/*
		Обертка оператора == поверх метода equal по метке устройства
	*/
	bool operator==(USB_DEVICE const &tmp);

	static bool deleteShortcut(const wchar_t* path);

	~USB_DEVICE()
	{
		deleteShortcut(strShortcutName);	
	}
};
