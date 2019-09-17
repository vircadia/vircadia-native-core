#include "Helper.h"
#ifdef Q_OS_WIN
#include <Windows.h>
#endif


#if defined(Q_OS_WIN)
void launchClient(const QString& homePath, const QString& defaultScriptOverride, const QString& displayName,
                  const QString& contentCachePath, const QString& loginResponseToken) {

    // TODO Fix parameters
    QString params = "--url " + homePath
        + " --setBookmark hqhome=\"" + homePath + "\""
        + " --defaultScriptsOverride " + defaultScriptsPath
        + " --displayName " + displayName
        + " --cache " + contentCachePath;

    if (!_loginTokenResponse.isEmpty()) {
        params += " --tokens \"" + _loginTokenResponse.replace("\"", "\\\"") + "\"";
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    // set the size of the structures
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // start the program up
    BOOL success = CreateProcess(
        clientPath.toUtf8().data(),
        params.toUtf8().data(),
        nullptr,                   // Process handle not inheritable
        nullptr,                   // Thread handle not inheritable
        FALSE,                  // Set handle inheritance to FALSE
        CREATE_NEW_CONSOLE,     // Opens file in a separate console
        nullptr,           // Use parent's environment block
        nullptr,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi           // Pointer to PROCESS_INFORMATION structure
    );
    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    exit(0);
}

#endif
