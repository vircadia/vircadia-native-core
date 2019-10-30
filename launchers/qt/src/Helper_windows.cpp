#include "Helper.h"

#include <QCoreApplication>

#include "windows.h"
#include "winnls.h"
#include "shobjidl.h"
#include "objbase.h"
#include "objidl.h"
#include "shlguid.h"
#include <atlstr.h>
#include <tlhelp32.h>
#include <strsafe.h>

void launchClient(const QString& clientPath, const QString& homePath, const QString& defaultScriptsPath,
                  const QString& displayName, const QString& contentCachePath, QString loginResponseToken) {

    // TODO Fix parameters
    QString params = "\"" + clientPath + "\"" + " --url \"" + homePath + "\""
        + " --setBookmark \"hqhome=" + homePath + "\""
        + " --defaultScriptsOverride \"file:///" + defaultScriptsPath + "\""
        + " --cache \"" + contentCachePath + "\""
        + " --suppress-settings-reset --no-launcher --no-updater";

    if (!displayName.isEmpty()) {
        params += " --displayName \"" + displayName + "\"";
    }

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
        clientPath.toLatin1().data(),
        params.toLatin1().data(),
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
}

void launchAutoUpdater(const QString& autoUpdaterPath) {
    QString params = "\"" + QCoreApplication::applicationFilePath() + "\"" + " --restart --noUpdate";
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

    QCoreApplication::instance()->quit();
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

bool insertRegistryKey(const std::string& regPath, const std::string& name, const std::string& value) {
    HKEY key;
    auto status = RegCreateKeyExA(HKEY_CURRENT_USER, regPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_QUERY_VALUE, NULL, &key, NULL);
    if (status == ERROR_SUCCESS) {
        status = RegSetValueExA(key, name.c_str(), 0, REG_SZ, (const BYTE*)value.c_str(), (DWORD)(value.size() + 1));
        return (bool) (status == ERROR_SUCCESS);
    }
    RegCloseKey(key);
    return false;
}

bool insertRegistryKey(const std::string& regPath, const std::string& name, DWORD value) {
    HKEY key;
    auto status = RegCreateKeyExA(HKEY_CURRENT_USER, regPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_QUERY_VALUE, NULL, &key, NULL);
    if (status == ERROR_SUCCESS) {
        status = RegSetValueExA(key, name.c_str(), 0, REG_DWORD, (const BYTE*)&value, sizeof(value));
        return (bool) TRUE;
    }
    RegCloseKey(key);
    return false;
}

bool deleteRegistryKey(const std::string& regPath) {
    TCHAR szDelKey[MAX_PATH * 2];
    StringCchCopy(szDelKey, MAX_PATH * 2, regPath.c_str());
    auto status = RegDeleteKey(HKEY_CURRENT_USER, szDelKey);
    if (status == ERROR_SUCCESS) {
        return (bool) TRUE;
    }
    return false;
}


BOOL isProcessRunning(const char* processName, int& processID) {
    bool exists = false;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry)) {
        while (Process32Next(snapshot, &entry)) {
            if (!_stricmp(entry.szExeFile, processName)) {
                exists = true;
                processID = entry.th32ProcessID;
                break;
            }
        }
    }
    CloseHandle(snapshot);
    return exists;
}

BOOL shutdownProcess(DWORD dwProcessId, UINT uExitCode) {
    DWORD dwDesiredAccess = PROCESS_TERMINATE;
    BOOL  bInheritHandle = FALSE;
    HANDLE hProcess = OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);
    if (hProcess == NULL) {
        return FALSE;
    }
    BOOL result = TerminateProcess(hProcess, uExitCode);
    CloseHandle(hProcess);
    return result;
}
