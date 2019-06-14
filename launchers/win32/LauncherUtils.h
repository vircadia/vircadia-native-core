//
//  LauncherUtils.h
//
//  Created by Luis Cuenca on 6/5/2019.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include <functional>

#include "libs/json/json.h"
#include "libs/miniz.h"

class LauncherUtils
{
public:
    enum ResponseError {
        Open = 0,
        Connect,
        OpenRequest,
        SendRequest,
        ReceiveRequest,
        ReadResponse,
        ParsingJSON,
        BadCredentials,
        NoError
    };

    enum DeleteDirError {
        NoErrorDeleting = 0,
        ErrorDeletingApplicationDir,
        ErrorDeletingDownloadsDir,
        ErrorDeletingBothDirs
    };

    struct DownloadThreadData {
        int _type;
        CString _url;
        CString _file;
        std::function<void(int)> callback;
        // function(type)
        void setCallback(std::function<void(int)> fn) {
            callback = std::bind(fn, std::placeholders::_1);
        }
    };

    struct UnzipThreadData {
        int _type;
        std::string _zipFile;
        std::string _path;
        // function(type, size)
        std::function<void(int, int)> callback;
        void setCallback(std::function<void(int, int)> fn) {
            callback = std::bind(fn, std::placeholders::_1, std::placeholders::_2);
        }
    };

    struct DeleteThreadData {
        CString _applicationDir;
        CString _downloadsDir;
        std::function<void(int)> callback;
        void setCallback(std::function<void(int)> fn) { callback = std::bind(fn, std::placeholders::_1); }
    };

    static BOOL parseJSON(const CString& jsonTxt, Json::Value& jsonObject);
    static ResponseError makeHTTPCall(const CString& callerName, const CString& mainUrl,
        const CString& dirUrl, const CString& contentType,
        CStringA& postData, CString& response, bool isPost);
    static std::string cStringToStd(CString cstring);
    static BOOL getFont(const CString& fontName, int fontSize, bool isBold, CFont& fontOut);
    static BOOL launchApplication(LPCWSTR lpApplicationName, LPTSTR cmdArgs = _T(""));
    static BOOL IsProcessRunning(const wchar_t *processName);
    static BOOL insertRegistryKey(const std::string& regPath, const std::string& name, const std::string& value);
    static BOOL insertRegistryKey(const std::string& regPath, const std::string& name, DWORD value);
    static BOOL deleteFileOrDirectory(const CString& dirPath, bool noRecycleBin = true);
    static HRESULT CreateLink(LPCWSTR lpszPathObj, LPCSTR lpszPathLink, LPCWSTR lpszDesc, LPCWSTR lpszArgs = _T(""));
    static BOOL hMac256(const CString& message, const char* key, CString& hashOut);
    static uint64_t extractZip(const std::string& zipFile, const std::string& path, std::vector<std::string>& files);
    static BOOL deleteRegistryKey(const CString& registryPath);
    static BOOL unzipFileOnThread(int type, const std::string& zipFile, const std::string& path, std::function<void(int, int)> callback);
    static BOOL downloadFileOnThread(int type, const CString& url, const CString& file, std::function<void(int)> callback);
    static BOOL deleteDirectoriesOnThread(const CString& applicationDir,
                                              const CString& downloadsDir,
                                              std::function<void(int)> callback);
    static CString urlEncodeString(const CString& url);
    static HWND executeOnForeground(const CString& path, const CString& params);

private:
    // Threads
    static DWORD WINAPI unzipThread(LPVOID lpParameter);
    static DWORD WINAPI downloadThread(LPVOID lpParameter);
    static DWORD WINAPI deleteDirectoriesThread(LPVOID lpParameter);
};