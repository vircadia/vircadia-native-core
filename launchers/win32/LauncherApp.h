//
//  LauncherApp.h
//
//  Created by Luis Cuenca on 6/5/2019.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "LauncherManager.h"

class CLauncherApp : public CWinApp
{
public:
	CLauncherApp();
	virtual BOOL InitInstance();
	void setDialogOnFront() { SetWindowPos(m_pMainWnd->GetSafeHwnd(), HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE); }
	LauncherManager _manager;
private:
	BOOL installFont(int fontID);	
	DECLARE_MESSAGE_MAP()
};

extern CLauncherApp theApp;
