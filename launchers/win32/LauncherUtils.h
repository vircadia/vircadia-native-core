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

class LauncherUtils {
public:
    class ProgressCallback : public IBindStatusCallback {
    public:
        HRESULT __stdcall QueryInterface(const IID &, void **) {
            return E_NOINTERFACE;
        }
        ULONG STDMETHODCALLTYPE AddRef(void) {
            return 1;
        }
        ULONG STDMETHODCALLTYPE Release(void) {
            return 1;
        }
        HRESULT STDMETHODCALLTYPE OnStartBinding(DWORD dwReserved, IBinding *pib) {
            return E_NOTIMPL;
        }
        virtual HRESULT STDMETHODCALLTYPE GetPriority(LONG *pnPriority) {
            return E_NOTIMPL;
        }
        virtual HRESULT STDMETHODCALLTYPE OnLowResource(DWORD reserved) {
            return S_OK;
        }
        virtual HRESULT STDMETHODCALLTYPE OnStopBinding(HRESULT hresult, LPCWSTR szError) {
            return E_NOTIMPL;
        }
        virtual HRESULT STDMETHODCALLTYPE GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo) {
            return E_NOTIMPL;
        }
        virtual HRESULT STDMETHODCALLTYPE OnDataAvailable(DWORD grfBSCF, DWORD dwSize, 
                                                          FORMATETC *pformatetc, STGMEDIUM *pstgmed) {
            return E_NOTIMPL;
        }
        virtual HRESULT STDMETHODCALLTYPE OnObjectAvailable(REFIID riid, IUnknown *punk) {
            return E_NOTIMPL;
        }

        virtual HRESULT __stdcall OnProgress(ULONG ulProgress, ULONG ulProgressMax, 
                                             ULONG ulStatusCode, LPCWSTR szStatusText) {
            float progress = (float)ulProgress / ulProgressMax;
            if (!isnan(progress)) {
                _onProgressCallback(progress);
            }
            return S_OK;
        }
        void setProgressCallback(std::function<void(float)> fn) {
            _onProgressCallback = std::bind(fn, std::placeholders::_1);
        }
    private:        
        std::function<void(float)> _onProgressCallback;
    };

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

    struct DownloadThreadData {
        int _type;
        CString _url;
        CString _file;
        std::function<void(int, bool)> _callback;
        std::function<void(float)> _progressCallback;
        // function(type, errorType)
        void setCallback(std::function<void(int, bool)> fn) {
            _callback = std::bind(fn, std::placeholders::_1, std::placeholders::_2);
        }
        void setProgressCallback(std::function<void(float)> fn) {
            _progressCallback = std::bind(fn, std::placeholders::_1);
        }
    };

    struct UnzipThreadData {
        int _type;
        std::string _zipFile;
        std::string _path;
        // function(type, size)
        std::function<void(int, int)> _callback;
        std::function<void(float)> _progressCallback;
        void setCallback(std::function<void(int, int)> fn) {
            _callback = std::bind(fn, std::placeholders::_1, std::placeholders::_2);
        }
        void setProgressCallback(std::function<void(float)> fn) {
            _progressCallback = std::bind(fn, std::placeholders::_1);
        }
    };

    struct DeleteThreadData {
        CString _dirPath;
        std::function<void(bool)> _callback;
        std::function<void(float)> _progressCallback;
        void setCallback(std::function<void(bool)> fn) { _callback = std::bind(fn, std::placeholders::_1); }
        void setProgressCallback(std::function<void(float)> fn) {
            _progressCallback = std::bind(fn, std::placeholders::_1);
        }
    };

    struct HttpThreadData {
        CString _callerName;
        bool _useHTTPS;
        CString _mainUrl; 
        CString _dirUrl;
        CString _contentType;
        CStringA _postData;
        bool _isPost { false };
        std::function<void(CString, int)> _callback;
        void setCallback(std::function<void(CString, int)> fn) { 
            _callback = std::bind(fn, std::placeholders::_1, std::placeholders::_2);
        }
    };

    struct ProcessData {
        int processID = -1;
        BOOL isOpened = FALSE;
    };

    static BOOL parseJSON(const CString& jsonTxt, Json::Value& jsonObject);
    static ResponseError makeHTTPCall(const CString& callerName, bool useHTTPS, const CString& mainUrl,
        const CString& dirUrl, const CString& contentType,
        CStringA& postData, CString& response, bool isPost);
    static std::string cStringToStd(CString cstring);
    static BOOL getFont(const CString& fontName, int fontSize, bool isBold, CFont& fontOut);
    static BOOL launchApplication(LPCWSTR lpApplicationName, LPTSTR cmdArgs = _T(""));
    static BOOL CALLBACK isWindowOpenedCallback(HWND hWnd, LPARAM lparam);
    static BOOL isProcessRunning(const wchar_t *processName, int& processID);
    static BOOL isProcessWindowOpened(const wchar_t *processName);
    static BOOL shutdownProcess(DWORD dwProcessId, UINT uExitCode);
    static BOOL insertRegistryKey(const std::string& regPath, const std::string& name, const std::string& value);
    static BOOL insertRegistryKey(const std::string& regPath, const std::string& name, DWORD value);
    static BOOL deleteFileOrDirectory(const CString& dirPath, bool noRecycleBin = true);
    static HRESULT createLink(LPCWSTR lpszPathObj, LPCSTR lpszPathLink, LPCWSTR lpszDesc, LPCWSTR lpszArgs = _T(""));
    static BOOL hMac256(const CString& message, const char* key, CString& hashOut);
    static uint64_t extractZip(const std::string& zipFile, const std::string& path,
                               std::vector<std::string>& files,
                               std::function<void(float)> progressCallback);
    static BOOL deleteRegistryKey(const CString& registryPath);
    static BOOL unzipFileOnThread(int type, const std::string& zipFile, const std::string& path, 
                                  std::function<void(int, int)> callback,
                                  std::function<void(float)> progressCallback);
    static BOOL downloadFileOnThread(int type, const CString& url, const CString& file, 
                                     std::function<void(int, bool)> callback,
                                     std::function<void(float)> progressCallback);
    static BOOL deleteDirectoryOnThread(const CString& dirPath, std::function<void(bool)> callback);
    static BOOL httpCallOnThread(const CString& callerName, bool useHTTPS, const CString& mainUrl, const CString& dirUrl,
                                 const CString& contentType, CStringA& postData, bool isPost,
                                 std::function<void(CString, int)> callback);

    static CString urlEncodeString(const CString& url);
    static HWND executeOnForeground(const CString& path, const CString& params);

private:
    // Threads
    static DWORD WINAPI unzipThread(LPVOID lpParameter);
    static DWORD WINAPI downloadThread(LPVOID lpParameter);
    static DWORD WINAPI deleteDirectoryThread(LPVOID lpParameter);
    static DWORD WINAPI httpThread(LPVOID lpParameter);
};
