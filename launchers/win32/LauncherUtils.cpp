//
//  LauncherUtils.cpp
//
//  Created by Luis Cuenca on 6/5/2019.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "stdafx.h"
#include <wincrypt.h>
#include <tlhelp32.h>
#include <strsafe.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp")

#include "LauncherApp.h"
#include "LauncherUtils.h"

CString LauncherUtils::urlEncodeString(const CString& url) {
    std::map<CString, CString> specialCharsMap = { { _T("$"), _T("%24") }, { _T(" "), _T("%20") }, { _T("#"), _T("%23") },
                                                   { _T("@"), _T("%40") }, { _T("`"), _T("%60") }, { _T("&"), _T("%26") },
                                                   { _T("/"), _T("%2F") }, { _T(":"), _T("%3A") }, { _T(";"), _T("%3B") },
                                                   { _T("<"), _T("%3C") }, { _T(">"), _T("%3E") }, { _T("="), _T("%3D") },
                                                   { _T("?"), _T("%3F") }, { _T("["), _T("%5B") }, { _T("\\"), _T("%5C") },
                                                   { _T("]"), _T("%5D") }, { _T("^"), _T("%5E") }, { _T("{"), _T("%7B") },
                                                   { _T("|"), _T("%7C") }, { _T("}"), _T("%7D") }, { _T("~"), _T("%7E") },
                                                   { _T("“"), _T("%22") }, { _T("‘"), _T("%27") }, { _T("+"), _T("%2B") },
                                                   { _T(","), _T("%2C") } };
    CString stringOut = url;
    stringOut.Replace(_T("%"), _T("%25"));
    for (auto& itr = specialCharsMap.begin(); itr != specialCharsMap.end(); itr++) {
        stringOut.Replace(itr->first, itr->second);
    }
    return stringOut;
}

BOOL LauncherUtils::IsProcessRunning(const wchar_t *processName) {
    bool exists = false;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry)) {
        while (Process32Next(snapshot, &entry)) {
            if (!_wcsicmp(entry.szExeFile, processName)) {
                exists = true;
                break;
            }
        }
    }
    CloseHandle(snapshot);
    return exists;
}

HRESULT LauncherUtils::CreateLink(LPCWSTR lpszPathObj, LPCSTR lpszPathLink, LPCWSTR lpszDesc, LPCWSTR lpszArgs) {
    IShellLink* psl;

    // Get a pointer to the IShellLink interface. It is assumed that CoInitialize
    // has already been called.
    CoInitialize(NULL);
    HRESULT hres = E_INVALIDARG;
    if ((lpszPathObj != NULL) && (wcslen(lpszPathObj) > 0) &&
        (lpszDesc != NULL) &&
        (lpszPathLink != NULL) && (strlen(lpszPathLink) > 0)) {
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
    }
    CoUninitialize();
    return SUCCEEDED(hres);
}

std::string LauncherUtils::cStringToStd(CString cstring) {
    CT2CA convertedString(cstring);
    std::string strStd(convertedString);
    return strStd;
}

BOOL LauncherUtils::launchApplication(LPCWSTR lpApplicationName, LPTSTR cmdArgs) {
    // additional information
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    // set the size of the structures
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // start the program up
    BOOL success = CreateProcess(
        lpApplicationName,   // the path
        cmdArgs,                // Command line
        NULL,                   // Process handle not inheritable
        NULL,                   // Thread handle not inheritable
        FALSE,                  // Set handle inheritance to FALSE
        CREATE_NEW_CONSOLE,     // Opens file in a separate console
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi           // Pointer to PROCESS_INFORMATION structure
    );
    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return success;
}

BOOL LauncherUtils::deleteRegistryKey(const CString& registryPath) {
    TCHAR szDelKey[MAX_PATH * 2];
    StringCchCopy(szDelKey, MAX_PATH * 2, registryPath);
    auto status = RegDeleteKey(HKEY_CURRENT_USER, szDelKey);
    if (status == ERROR_SUCCESS) {
        return TRUE;
    }
    return FALSE;
}

LauncherUtils::ResponseError LauncherUtils::makeHTTPCall(const CString& callerName,
    const CString& mainUrl, const CString& dirUrl,
    const CString& contentType, CStringA& postData,
    CString& response, bool isPost = false) {

    HINTERNET hopen = WinHttpOpen(callerName, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hopen) {
        return ResponseError::Open;
    }
    HINTERNET hconnect = WinHttpConnect(hopen, mainUrl, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hconnect) {
        return ResponseError::Connect;
    }
    HINTERNET hrequest = WinHttpOpenRequest(hconnect, isPost ? L"POST" : L"GET", dirUrl,
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hrequest) {
        return ResponseError::OpenRequest;
    }
    if (isPost) {
        if (!WinHttpSendRequest(hrequest, contentType, -1,
            postData.GetBuffer(postData.GetLength()), (DWORD)strlen(postData), (DWORD)strlen(postData), NULL)) {
            return ResponseError::SendRequest;
        }
    } else {
        if (!WinHttpSendRequest(hrequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
            return ResponseError::SendRequest;
        }
    }
    if (!WinHttpReceiveResponse(hrequest, 0)) {
        return ResponseError::ReceiveRequest;
    }
    // query remote file size, set haveContentLength on success and dwContentLength to the length
    wchar_t szContentLength[32];
    DWORD bufferBytes = 4096;
    DWORD dwHeaderIndex = WINHTTP_NO_HEADER_INDEX;

    BOOL haveContentLength = WinHttpQueryHeaders(hrequest, WINHTTP_QUERY_CONTENT_LENGTH, NULL,
        &szContentLength, &bufferBytes, &dwHeaderIndex);

    DWORD statusCode;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hrequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        NULL,
        &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);

    CString msg;
    msg.Format(_T("Status code response (%s%s): %lu"), mainUrl, dirUrl, statusCode);
    theApp._manager.addToLog(msg);

    DWORD dwContentLength;
    if (haveContentLength) {
        dwContentLength = _wtoi(szContentLength);
    }
    byte p[4096];
    if (!WinHttpQueryDataAvailable(hrequest, &bufferBytes)) {
        return ResponseError::ReadResponse;
    }
    WinHttpReadData(hrequest, p, bufferBytes, &bufferBytes);
    response = CString((const char*)p, (int)bufferBytes);
    return ResponseError::NoError;
}

BOOL LauncherUtils::parseJSON(const CString& jsonTxt, Json::Value& root) {
    Json::CharReaderBuilder CharBuilder;
    std::string jsonString = cStringToStd(jsonTxt);
    std::string errs;
    Json::CharReader* nreader = CharBuilder.newCharReader();
    bool parsingSuccessful = false;
    if (nreader != NULL) {
        parsingSuccessful = nreader->parse(jsonString.c_str(), jsonString.c_str() + jsonString.length(), &root, &errs);
        delete nreader;
    }
    return parsingSuccessful;
}

BOOL LauncherUtils::getFont(const CString& fontName, int fontSize, bool isBold, CFont& fontOut) {
    LOGFONT lf;
    memset(&lf, 0, sizeof(lf));
    lf.lfHeight = fontSize;
    lf.lfWeight = isBold ? FW_BOLD : FW_NORMAL;
    lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
    lf.lfQuality = ANTIALIASED_QUALITY;
    
    wcscpy_s(lf.lfFaceName, fontName);
    if (!fontOut.CreateFontIndirect(&lf)) {
        return FALSE;
    }
    return TRUE;
}

uint64_t LauncherUtils::extractZip(const std::string& zipFile, const std::string& path, std::vector<std::string>& files) {
    {
        CString msg;
        msg.Format(_T("Reading zip file %s, extracting to %s"), CString(zipFile.c_str()), CString(path.c_str()));
        theApp._manager.addToLog(msg);
    }

    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    auto status = mz_zip_reader_init_file(&zip_archive, zipFile.c_str(), 0);

    if (!status) {
        auto zip_error = mz_zip_get_last_error(&zip_archive);
        auto zip_error_msg = mz_zip_get_error_string(zip_error);
        CString msg;
        msg.Format(_T("Failed to initialize miniz: %d %s"), zip_error, CString(zip_error_msg));
        theApp._manager.addToLog(msg);
        return 0;
    }

    int fileCount = (int)mz_zip_reader_get_num_files(&zip_archive);
    if (fileCount == 0) {
        theApp._manager.addToLog(_T("Zip archive has a file count of 0"));

        mz_zip_reader_end(&zip_archive);
        return 0;
    }
    mz_zip_archive_file_stat file_stat;
    if (!mz_zip_reader_file_stat(&zip_archive, 0, &file_stat)) {
        theApp._manager.addToLog(_T("Zip archive cannot be stat'd"));

        mz_zip_reader_end(&zip_archive);
        return 0;
    }
    // Get root folder
    CString lastDir = _T("");
    uint64_t totalSize = 0;
    for (int i = 0; i < fileCount; i++) {
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) continue;
        std::string filename = file_stat.m_filename;
        std::replace(filename.begin(), filename.end(), '/', '\\');
        CString fullFilename = CString(path.c_str()) + "\\" + CString(filename.c_str());
        if (mz_zip_reader_is_file_a_directory(&zip_archive, i)) {
            if (SHCreateDirectoryEx(NULL, fullFilename, NULL) || ERROR_ALREADY_EXISTS == GetLastError()) {
                break;
            } else {
                continue;
            }
        }
        CT2A destFile(fullFilename);
        if (mz_zip_reader_extract_to_file(&zip_archive, i, destFile, 0)) {
            totalSize += (uint64_t)file_stat.m_uncomp_size;
            files.emplace_back(destFile);
        }
    }

    {
        CString msg;
        msg.Format(_T("Done unzipping archive, total size: %llu"), totalSize);
        theApp._manager.addToLog(msg);
    }

    // Close the archive, freeing any resources it was using
    mz_zip_reader_end(&zip_archive);
    return totalSize;
}

BOOL LauncherUtils::insertRegistryKey(const std::string& regPath, const std::string& name, const std::string& value) {
    HKEY key;
    auto status = RegCreateKeyExA(HKEY_CURRENT_USER, regPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_QUERY_VALUE, NULL, &key, NULL);
    if (status == ERROR_SUCCESS) {
        status = RegSetValueExA(key, name.c_str(), 0, REG_SZ, (const BYTE*)value.c_str(), (DWORD)(value.size() + 1));
        return status == ERROR_SUCCESS;
    }
    RegCloseKey(key);
    return FALSE;
}

BOOL LauncherUtils::insertRegistryKey(const std::string& regPath, const std::string& name, DWORD value) {
    HKEY key;
    auto status = RegCreateKeyExA(HKEY_CURRENT_USER, regPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_QUERY_VALUE, NULL, &key, NULL);
    if (status == ERROR_SUCCESS) {
        status = RegSetValueExA(key, name.c_str(), 0, REG_DWORD, (const BYTE*)&value, sizeof(value));
        return TRUE;
    }
    RegCloseKey(key);
    return FALSE;
}

BOOL LauncherUtils::deleteFileOrDirectory(const CString& dirPath, bool noRecycleBin) {
    CString dir = dirPath;
    // Add extra null to string
    dir.AppendChar(0);
    SHFILEOPSTRUCT fileop;
    fileop.hwnd = NULL;    // no status display
    fileop.wFunc = FO_DELETE;  // delete operation
    fileop.pFrom = dir;  // source file name as double null terminated string
    fileop.pTo = NULL;    // no destination needed
    fileop.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;  // do not prompt the user

    if (!noRecycleBin) {
        fileop.fFlags |= FOF_ALLOWUNDO;
    }

    fileop.fAnyOperationsAborted = FALSE;
    fileop.lpszProgressTitle = NULL;
    fileop.hNameMappings = NULL;

    int ret = SHFileOperation(&fileop);
    return (ret == 0);
}

BOOL LauncherUtils::hMac256(const CString& cmessage, const char* keystr, CString& hashOut) {
    char message[256];
    strcpy_s(message, CStringA(cmessage).GetString());
    char key[256];
    strcpy_s(key, keystr);
    HCRYPTPROV  hProv = 0;
    HCRYPTHASH  hHash = 0;
    HCRYPTKEY   hKey = 0;
    HCRYPTHASH  hHmacHash = 0;
    BYTE pbHash[32];
    HMAC_INFO   HmacInfo;
    int err = 0;
    typedef struct blob {
        BLOBHEADER header;
        DWORD len;
        BYTE key[1];
    } m_blob;

    ZeroMemory(&HmacInfo, sizeof(HmacInfo));
    HmacInfo.HashAlgid = CALG_SHA_256;
    ZeroMemory(&pbHash, 32);
    m_blob* kb = NULL;
    DWORD kbSize = DWORD(sizeof(m_blob) + strlen(key));

    kb = (m_blob*)malloc(kbSize);
    kb->header.bType = PLAINTEXTKEYBLOB;
    kb->header.bVersion = CUR_BLOB_VERSION;
    kb->header.reserved = 0;
    kb->header.aiKeyAlg = CALG_RC2;
    memcpy(&kb->key, key, strlen(key));
    kb->len = (DWORD)strlen(key);
    BOOL success = false;
    DWORD datalen = (DWORD)32;
    if (CryptAcquireContext(&hProv, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_NEWKEYSET)
        && CryptImportKey(hProv, (BYTE*)kb, kbSize, 0, CRYPT_IPSEC_HMAC_KEY, &hKey)
        && CryptCreateHash(hProv, CALG_HMAC, hKey, 0, &hHmacHash)
        && CryptSetHashParam(hHmacHash, HP_HMAC_INFO, (BYTE*)&HmacInfo, 0)
        && CryptHashData(hHmacHash, (BYTE*)message, (DWORD)strlen(message), 0)
        && CryptGetHashParam(hHmacHash, HP_HASHVAL, pbHash, &datalen, 0)) {
        char *Hex = "0123456789abcdef";
        char HashString[65] = { 0 };
        for (int Count = 0; Count < 32; Count++)
        {
            HashString[Count * 2] = Hex[pbHash[Count] >> 4];
            HashString[(Count * 2) + 1] = Hex[pbHash[Count] & 0xF];
        }
        hashOut = CString(HashString);
        success = true;
    }

    free(kb);
    if (hHmacHash)
        CryptDestroyHash(hHmacHash);
    if (hKey)
        CryptDestroyKey(hKey);
    if (hHash)
        CryptDestroyHash(hHash);
    if (hProv)
        CryptReleaseContext(hProv, 0);
    return success;
}


DWORD WINAPI LauncherUtils::unzipThread(LPVOID lpParameter) {
    UnzipThreadData& data = *((UnzipThreadData*)lpParameter);
    uint64_t size = LauncherUtils::extractZip(data._zipFile, data._path, std::vector<std::string>());
    int mb_size = (int)(size * 0.001f);
    data.callback(data._type, mb_size);
    delete &data;
    return 0;
}

DWORD WINAPI LauncherUtils::downloadThread(LPVOID lpParameter)
{
    DownloadThreadData& data = *((DownloadThreadData*)lpParameter);
    auto hr = URLDownloadToFile(0, data._url, data._file, 0, NULL);
    data.callback(data._type);
    return 0;
}

DWORD WINAPI LauncherUtils::deleteDirectoriesThread(LPVOID lpParameter) {
    DeleteThreadData& data = *((DeleteThreadData*)lpParameter);
    DeleteDirError error = DeleteDirError::NoErrorDeleting;
    if (!LauncherUtils::deleteFileOrDirectory(data._applicationDir)) {
        error = DeleteDirError::ErrorDeletingApplicationDir;
    }
    if (!LauncherUtils::deleteFileOrDirectory(data._downloadsDir)) {
        error = error == NoError ? DeleteDirError::ErrorDeletingDownloadsDir : DeleteDirError::ErrorDeletingBothDirs;
    }
    data.callback(error);
    return 0;
}

BOOL LauncherUtils::unzipFileOnThread(int type, const std::string& zipFile, const std::string& path, std::function<void(int, int)> callback) {
    DWORD myThreadID;
    UnzipThreadData* unzipThreadData = new UnzipThreadData();
    unzipThreadData->_type = type;
    unzipThreadData->_zipFile = zipFile;
    unzipThreadData->_path = path;
    unzipThreadData->setCallback(callback);
    HANDLE myHandle = CreateThread(0, 0, unzipThread, unzipThreadData, 0, &myThreadID);
    if (myHandle) {
        CloseHandle(myHandle);
        return TRUE;
    }
    return FALSE;
}

BOOL LauncherUtils::downloadFileOnThread(int type, const CString& url, const CString& file, std::function<void(int)> callback) {
    DWORD myThreadID;
    DownloadThreadData* downloadThreadData = new DownloadThreadData();
    downloadThreadData->_type = type;
    downloadThreadData->_url = url;
    downloadThreadData->_file = file;
    downloadThreadData->setCallback(callback);
    HANDLE myHandle = CreateThread(0, 0, downloadThread, downloadThreadData, 0, &myThreadID);
    if (myHandle) {
        CloseHandle(myHandle);
        return TRUE;
    }
    return FALSE;
}

BOOL LauncherUtils::deleteDirectoriesOnThread(const CString& applicationDir,
                                              const CString& downloadsDir,
                                              std::function<void(int)> callback) {
    DWORD myThreadID;
    DeleteThreadData* deleteThreadData = new DeleteThreadData();
    deleteThreadData->_applicationDir = applicationDir;
    deleteThreadData->_downloadsDir = downloadsDir;
    deleteThreadData->setCallback(callback);
    HANDLE myHandle = CreateThread(0, 0, deleteDirectoriesThread, deleteThreadData, 0, &myThreadID);
    if (myHandle) {
        CloseHandle(myHandle);
        return TRUE;
    }
    return FALSE;
}

HWND LauncherUtils::executeOnForeground(const CString& path, const CString& params) {
    SHELLEXECUTEINFO info;
    info.cbSize = sizeof(SHELLEXECUTEINFO);
    info.lpVerb = _T("open");
    info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC | SEE_MASK_WAITFORINPUTIDLE;
    info.hwnd = NULL;
    info.lpVerb = NULL;
    info.lpParameters = NULL;
    info.lpDirectory = NULL;
    info.nShow = SW_SHOWNORMAL;
    info.hInstApp = NULL;
    info.lpFile = path;
    info.lpParameters = params;
    HWND hwnd = NULL;
    if (!ShellExecuteEx(&info)) {
        return FALSE;
    } else {
        DWORD infopid = GetProcessId(info.hProcess);
        AllowSetForegroundWindow(infopid);
        hwnd = GetTopWindow(0);
        while (hwnd) {
            DWORD pid;
            DWORD dwTheardId = ::GetWindowThreadProcessId(hwnd, &pid);
            if (pid == infopid) {
                SetForegroundWindow(hwnd);
                SetActiveWindow(hwnd);
                break;
            }
            hwnd = ::GetNextWindow(hwnd, GW_HWNDNEXT);
        }
        CloseHandle(info.hProcess);
    }
    return hwnd;
}
