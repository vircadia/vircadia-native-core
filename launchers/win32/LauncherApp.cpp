//
//  LauncherApp.cpp
//
//  Created by Luis Cuenca on 6/5/2019.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "stdafx.h"
#include "LauncherApp.h"
#include "LauncherDlg.h"
#include "LauncherManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CLauncherApp

BEGIN_MESSAGE_MAP(CLauncherApp, CWinApp)
    ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

CLauncherApp::CLauncherApp(){}

// The one and only CLauncherApp object

CLauncherApp theApp;

// CLauncherApp initialization

BOOL CLauncherApp::InitInstance() {
    // don't launch if already running
    CreateMutex(NULL, TRUE, _T("HQ_Launcher_Mutex"));
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return FALSE;
    }
    int iNumOfArgs;
    LPWSTR* pArgs = CommandLineToArgvW(GetCommandLine(), &iNumOfArgs);
    if (iNumOfArgs > 1 && CString(pArgs[1]).Compare(_T("--uninstall")) == 0) {
        _manager.uninstall();
    } else {
        _manager.init();
    }   
    if (!_manager.installLauncher()) {
        return FALSE;
    }
    installFont(IDR_FONT_REGULAR);
    installFont(IDR_FONT_BOLD);
    CWinApp::InitInstance();

    SetRegistryKey(_T("HQ High Fidelity"));

    CLauncherDlg dlg;
    m_pMainWnd = &dlg;
    INT_PTR nResponse = dlg.DoModal();      

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
    ControlBarCleanUp();
#endif
    
    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}



BOOL CLauncherApp::installFont(int fontID) {
    HINSTANCE hResInstance = AfxGetResourceHandle();
    HRSRC res = FindResource(hResInstance,
        MAKEINTRESOURCE(fontID), L"BINARY");
    if (res) {
        HGLOBAL mem = LoadResource(hResInstance, res);
        void *data = LockResource(mem);
        DWORD len = (DWORD)SizeofResource(hResInstance, res);

        DWORD nFonts;
        auto m_fonthandle = AddFontMemResourceEx(
            data,           // font resource
            len,        // number of bytes in font resource 
            NULL,           // Reserved. Must be 0.
            &nFonts         // number of fonts installed
        );

        return (m_fonthandle != 0);
    }
    return FALSE;
}
