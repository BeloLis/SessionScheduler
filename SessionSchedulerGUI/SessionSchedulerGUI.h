
// SessionSchedulerGUI.h: главный файл заголовка для приложения PROJECT_NAME
//

#pragma once

#ifndef __AFXWIN_H__
	#error "включить pch.h до включения этого файла в PCH"
#endif

#include "resource.h"		// основные символы


// CSessionSchedulerGUIApp:
// Сведения о реализации этого класса: SessionSchedulerGUI.cpp
//

class CSessionSchedulerGUIApp : public CWinApp
{
public:
	CSessionSchedulerGUIApp();

// Переопределение
public:
	virtual BOOL InitInstance();

// Реализация

	DECLARE_MESSAGE_MAP()
};

extern CSessionSchedulerGUIApp theApp;
