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
    // Close interface if is running
    int interfacePID = -1;
    if (LauncherUtils::isProcessRunning(L"interface.exe", interfacePID)) {
        LauncherUtils::shutdownProcess(interfacePID, 0);
    }
    int iNumOfArgs;
    LPWSTR* pArgs = CommandLineToArgvW(GetCommandLine(), &iNumOfArgs);
    bool uninstalling = false;
    bool restarting = false;
    bool noUpdate = false;
    LauncherManager::ContinueActionOnStart continueAction = LauncherManager::ContinueActionOnStart::ContinueNone;
    if (iNumOfArgs > 1) {
        for (int i = 1; i < iNumOfArgs; i++) {
            CString curArg = CString(pArgs[i]);
            if (curArg.Compare(_T("--uninstall")) == 0) {
                uninstalling = true;
            } else if (curArg.Compare(_T("--restart")) == 0) {
                restarting = true;
            } else if (curArg.Compare(_T("--noUpdate")) == 0) {
                noUpdate = true;
            } else if (curArg.Compare(_T("--continueAction")) == 0) {
                if (i + 1 < iNumOfArgs) {
                    continueAction = LauncherManager::getContinueActionFromParam(pArgs[i + 1]);
                }
            }
        }
    }
    if (!restarting) {
        // don't launch if already running
        CreateMutex(NULL, TRUE, _T("HQ_Launcher_Mutex"));
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            return FALSE;
        }
    }

    if (uninstalling) {
        _manager.uninstall();
    } else {
        _manager.init(!noUpdate, continueAction);
    }   
    _manager.tryToInstallLauncher();
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
