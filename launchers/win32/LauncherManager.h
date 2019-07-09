//
//  LauncherManager.h
//
//  Created by Luis Cuenca on 6/5/2019.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include "LauncherUtils.h"

const CString DIRECTORY_NAME_APP = _T("HQ");
const CString DIRECTORY_NAME_DOWNLOADS = _T("downloads");
const CString DIRECTORY_NAME_INTERFACE = _T("interface");
const CString DIRECTORY_NAME_CONTENT = _T("content");
const CString EXTRA_PARAMETERS = _T(" --suppress-settings-reset --no-launcher --no-updater");
const CString LAUNCHER_EXE_FILENAME = _T("HQ Launcher.exe");
const bool INSTALL_ZIP = true;
const float DOWNLOAD_CONTENT_INSTALL_WEIGHT = 0.2f;
const float EXTRACT_CONTENT_INSTALL_WEIGHT = 0.1f;
const float DOWNLOAD_APPLICATION_INSTALL_WEIGHT = 0.5f;
const float EXTRACT_APPLICATION_INSTALL_WEIGHT = 0.2f;
const float DOWNLOAD_APPLICATION_UPDATE_WEIGHT = 0.75f;
const float EXTRACT_APPLICATION_UPDATE_WEIGHT = 0.25f;

class LauncherManager
{
public:
    enum PathType {
        Running_Path = 0,
        Launcher_Directory,
        Download_Directory,
        Interface_Directory,
        Desktop_Directory,
        Content_Directory,
        StartMenu_Directory,
        Temp_Directory
    };
    enum ErrorType {
        ErrorNetworkAuth = 0,
        ErrorNetworkUpdate,
        ErrorNetworkHq,
        ErrorDownloading,
        ErrorUpdating,
        ErrorInstall,
        ErrorIOFiles
    };
    enum ProcessType {
        DownloadContent,
        DownloadApplication,
        UnzipContent,
        UnzipApplication
    };
    LauncherManager();
    ~LauncherManager();
    void init();
    BOOL initLog();
    BOOL addToLog(const CString& line);
    void closeLog();
    void saveErrorLog();
    BOOL getAndCreatePaths(PathType type, CString& outPath);
    BOOL getInstalledVersion(const CString& path, CString& version);
    BOOL isApplicationInstalled(CString& version, CString& domain,
                                CString& content, bool& loggedIn);
    LauncherUtils::ResponseError getAccessTokenForCredentials(const CString& username, const CString& password);
    LauncherUtils::ResponseError getMostRecentBuild(CString& urlOut, CString& versionOut, CString& response);
    LauncherUtils::ResponseError readOrganizationJSON(const CString& hash);
    LauncherUtils::ResponseError readConfigJSON(CString& version, CString& domain,
                                                CString& content, bool& loggedIn);
    BOOL createConfigJSON();
    BOOL createApplicationRegistryKeys(int size);
    BOOL deleteApplicationRegistryKeys();
    BOOL createShortcuts();
    BOOL deleteShortcuts();
    HWND launchApplication();
    BOOL uninstallApplication();
    BOOL installLauncher();
    BOOL restartLauncher();

    //  getters
    const CString& getContentURL() const { return _contentURL; }
    const CString& getdomainURL() const { return _domainURL; }
    const CString& getVersion() const { return _version; }
    BOOL shouldShutDown() const { return _shouldShutdown; }
    BOOL shouldLaunch() const { return _shouldLaunch; }
    BOOL needsUpdate() { return _shouldUpdate; }
    BOOL needsUninstall() { return _shouldUninstall; }
    BOOL needsInstall() { return _shouldInstall; }
    void setDisplayName(const CString& displayName) { _displayName = displayName; }
    bool isLoggedIn() { return _loggedIn; }
    bool hasFailed() { return _hasFailed; }
    void setFailed(bool hasFailed) { _hasFailed = hasFailed; }
    const CString& getLatestInterfaceURL() const { return _latestApplicationURL; }
    void uninstall() { _shouldUninstall = true; };

    BOOL downloadFile(ProcessType type, const CString& url, CString& localPath);
    BOOL downloadContent();
    BOOL downloadApplication();
    BOOL installContent();
    BOOL extractApplication();
    void onZipExtracted(ProcessType type, int size);
    void onFileDownloaded(ProcessType type);
    float getProgress() { return _progress; }

private:
    void updateProgress(ProcessType processType, float progress);
    ProcessType _currentProcess { ProcessType::DownloadApplication };
    CString _latestApplicationURL;
    CString _latestVersion;
    CString _contentURL;
    CString _domainURL;
    CString _version;
    CString _displayName;
    CString _tokensJSON;
    CString _applicationZipPath;
    CString _contentZipPath;
    bool _loggedIn { false };
    bool _hasFailed { false };
    BOOL _shouldUpdate { FALSE };
    BOOL _shouldUninstall { FALSE };
    BOOL _shouldInstall { FALSE };
    BOOL _shouldShutdown { FALSE };
    BOOL _shouldLaunch { FALSE };
    float _progress { 0.0f };
    CStdioFile _logFile;
};

