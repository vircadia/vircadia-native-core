#include "Helper.h"

#include "windows.h"
#include "winnls.h"
#include "shobjidl.h"
#include "objbase.h"
#include "objidl.h"
#include "shlguid.h"
#include <QCoreApplication>

void launchClient(const QString& clientPath, const QString& homePath, const QString& defaultScriptsPath,
                  const QString& displayName, const QString& contentCachePath, QString loginResponseToken) {

    // TODO Fix parameters
    QString params = "--url \"" + homePath + "\""
        + " --setBookmark \"hqhome=" + homePath + "\""
        + " --defaultScriptsOverride \"file:///" + defaultScriptsPath + "\""
        + " --displayName \"" + displayName + "\""
        + " --cache \"" + contentCachePath + "\"";

    if (!loginResponseToken.isEmpty()) {
        params += " --tokens \"" + loginResponseToken.replace("\"", "\\\"") + "\"";
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
        nullptr,                 // Process handle not inheritable
        nullptr,                 // Thread handle not inheritable
        FALSE,                   // Set handle inheritance to FALSE
        CREATE_NEW_CONSOLE,      // Opens file in a separate console
        nullptr,                 // Use parent's environment block
        nullptr,                 // Use parent's starting directory
        &si,                     // Pointer to STARTUPINFO structure
        &pi                      // Pointer to PROCESS_INFORMATION structure
    );

    // Close process and thread handles.
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    QCoreApplication::instance()->quit();
}

void launchAutoUpdater(const QString& autoUpdaterPath) {
    QString params = "--restart --noUpdate";
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    BOOL success = CreateProcess(
        autoUpdaterPath.toUtf8().data(),
        params.toUtf8().data(),
        nullptr,                // Process handle not inheritable
        nullptr,                // Thread handle not inheritable
        FALSE,                  // Set handle inheritance to FALSE
        CREATE_NEW_CONSOLE,     // Opens file in a separate console
        nullptr,                // Use parent's environment block
        nullptr,                // Use parent's starting directory
        &si,                    // Pointer to STARTUPINFO structure
        &pi                     // Pointer to PROCESS_INFORMATION structure
    );
}


HRESULT createSymbolicLink(LPCSTR lpszPathObj, LPCSTR lpszPathLink, LPCSTR lpszDesc, LPCSTR lpszArgs) {
    IShellLink* psl;

    // Get a pointer to the IShellLink interface. It is assumed that CoInitialize
    // has already been called.
    CoInitialize(NULL);
    HRESULT hres = E_INVALIDARG;
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if (SUCCEEDED(hres)) {
        IPersistFile* ppf;

        // Set the path to the shortcut target and add the description.
        psl->SetPath(lpszPathObj);
        psl->SetDescription(lpszDesc);
        psl->SetArguments(lpszArgs);

        // Query IShellLink for the IPersistFile interface, used for saving the
        // shortcut in persistent storage.
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);

        if (SUCCEEDED(hres)) {
            WCHAR wsz[MAX_PATH];

            // Ensure that the string is Unicode.
            MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1, wsz, MAX_PATH);

            // Add code here to check return value from MultiByteWideChar
            // for success.

            // Save the link by calling IPersistFile::Save.
            hres = ppf->Save(wsz, TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    CoUninitialize();
    return SUCCEEDED(hres);
}
