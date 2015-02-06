//
//  main.cpp
//  interface/src
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QTranslator>

#include <SharedUtil.h>

#include "AddressManager.h"
#include "Application.h"

#ifdef Q_OS_WIN
static BOOL CALLBACK enumWindowsCallback(HWND hWnd, LPARAM lParam) {
    const UINT TIMEOUT = 200;  // ms
    DWORD response;
    LRESULT result = SendMessageTimeout(hWnd, UWM_IDENTIFY_INSTANCES, 0, 0, SMTO_BLOCK | SMTO_ABORTIFHUNG, TIMEOUT, &response);
    if (result == 0) {  // Timeout; continue search.
        return TRUE;
    }
    if (response == UWM_IDENTIFY_INSTANCES) {
        HWND* target = (HWND*)lParam;
        *target = hWnd;
        return FALSE;  // Found; terminate search.
    }
    return TRUE;  // Not found; continue search.
}
#endif


int main(int argc, const char * argv[]) {
    Q_INIT_RESOURCE(fonts);

#ifdef Q_OS_WIN
    // Run only one instance of Interface at a time.
    HANDLE mutex = CreateMutex(NULL, FALSE, "High Fidelity Interface");
    DWORD result = GetLastError();
    if (result == ERROR_ALREADY_EXISTS || result == ERROR_ACCESS_DENIED) {
        // Interface is already running.
        HWND otherInstance = NULL;
        EnumWindows(enumWindowsCallback, (LPARAM)&otherInstance);
        if (otherInstance) {
            ShowWindow(otherInstance, SW_RESTORE);
            SetForegroundWindow(otherInstance);

            QUrl url = QUrl(argv[1]);
            if (url.isValid() && url.scheme() == HIFI_URL_SCHEME) {
                COPYDATASTRUCT cds;
                cds.cbData = strlen(argv[1]) + 1;
                cds.lpData = (PVOID)argv[1];
                SendMessage(otherInstance, WM_COPYDATA, 0, (LPARAM)&cds);
            }
        }
        return 0;
    }
#endif

    QElapsedTimer startupTime;
    startupTime.start();
    
    // Debug option to demonstrate that the client's local time does not 
    // need to be in sync with any other network node. This forces clock 
    // skew for the individual client
    const char* CLOCK_SKEW = "--clockSkew";
    const char* clockSkewOption = getCmdOption(argc, argv, CLOCK_SKEW);
    if (clockSkewOption) {
        int clockSkew = atoi(clockSkewOption);
        usecTimestampNowForceClockSkew(clockSkew);
        qDebug("clockSkewOption=%s clockSkew=%d", clockSkewOption, clockSkew);
    }
    
    int exitCode;
    {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        Application app(argc, const_cast<char**>(argv), startupTime);

        QTranslator translator;
        translator.load("interface_en");
        app.installTranslator(&translator);
    
        qDebug( "Created QT Application.");
        exitCode = app.exec();
    }

#ifdef Q_OS_WIN
    ReleaseMutex(mutex);
#endif

    qDebug("Normal exit.");
    return exitCode;
}   
