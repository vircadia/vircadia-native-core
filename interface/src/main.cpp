//
//  main.cpp
//  interface/src
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QTranslator>

#include <SharedUtil.h>

#include "AddressManager.h"
#include "Application.h"
#include "InterfaceLogging.h"

#ifdef Q_OS_WIN
static BOOL CALLBACK enumWindowsCallback(HWND hWnd, LPARAM lParam) {
    const UINT TIMEOUT = 200;  // ms
    DWORD_PTR response;
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

int main(int argc, const char* argv[]) {
#ifdef Q_OS_WIN
    // Run only one instance of Interface at a time.
    HANDLE mutex = CreateMutex(NULL, FALSE, "High Fidelity Interface - " + qgetenv("USERNAME"));
    DWORD result = GetLastError();
    if (result == ERROR_ALREADY_EXISTS || result == ERROR_ACCESS_DENIED) {
        // Interface is already running.
        HWND otherInstance = NULL;
        EnumWindows(enumWindowsCallback, (LPARAM)&otherInstance);
        if (otherInstance) {
            // Show other instance.
            SendMessage(otherInstance, UWM_SHOW_APPLICATION, 0, 0);

            // Send command line --url value to other instance.
            if (argc >= 3) {
                QStringList arguments;
                for (int i = 0; i < argc; i += 1) {
                    arguments << argv[i];
                }

                QCommandLineParser parser;
                QCommandLineOption urlOption("url", "", "value");
                parser.addOption(urlOption);
                parser.process(arguments);

                if (parser.isSet(urlOption)) {
                    QUrl url = QUrl(parser.value(urlOption));
                    if (url.isValid() && url.scheme() == HIFI_URL_SCHEME) {
                        QByteArray urlBytes = url.toString().toLatin1();
                        const char* urlChars = urlBytes.data();
                        COPYDATASTRUCT cds;
                        cds.cbData = urlBytes.length() + 1;
                        cds.lpData = (PVOID)urlChars;
                        SendMessage(otherInstance, WM_COPYDATA, 0, (LPARAM)&cds);
                    }
                }
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
        qCDebug(interfaceapp, "clockSkewOption=%s clockSkew=%d", clockSkewOption, clockSkew);
    }
    // Oculus initialization MUST PRECEDE OpenGL context creation.
    // The nature of the Application constructor means this has to be either here,
    // or in the main window ctor, before GL startup.
    Application::initPlugins();

    int exitCode;
    {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        Application app(argc, const_cast<char**>(argv), startupTime);

        QTranslator translator;
        translator.load("i18n/interface_en");
        app.installTranslator(&translator);
    
        qCDebug(interfaceapp, "Created QT Application.");
        exitCode = app.exec();
    }

    Application::shutdownPlugins();
#ifdef Q_OS_WIN
    ReleaseMutex(mutex);
#endif

    qCDebug(interfaceapp, "Normal exit.");
    return exitCode;
}   
