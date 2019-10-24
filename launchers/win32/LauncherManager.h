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
#include "LauncherDlg.h"

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
const float CONTINUE_UPDATING_GLOBAL_OFFSET = 0.2f;

class LauncherManager {
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
        DownloadLauncher = 0,
        DownloadContent,
        DownloadApplication,
        UnzipContent,
        UnzipApplication,
        Uninstall
    };
    enum ContinueActionOnStart {
        ContinueNone = 0,
        ContinueLogIn,
        ContinueUpdate,
        ContinueFinish
    };

    LauncherManager();
    ~LauncherManager();
    void init(BOOL allowUpdate, ContinueActionOnStart continueAction);
    static CString getContinueActionParam(ContinueActionOnStart continueAction);
    static ContinueActionOnStart getContinueActionFromParam(const CString& param);
    BOOL initLog();
    BOOL addToLog(const CString& line);
    void closeLog();
    void saveErrorLog();
    BOOL getAndCreatePaths(PathType type, CString& outPath);
    BOOL getInstalledVersion(const CString& path, CString& version);
    BOOL isApplicationInstalled(CString& version, CString& domain, 
                                CString& content, bool& loggedIn, CString& organizationBuildTag);
    LauncherUtils::ResponseError getAccessTokenForCredentials(const CString& username, const CString& password);
    void getMostRecentBuilds(CString& launcherUrlOut,
                             CString& launcherVersionOut,
                             CString& interfaceUrlOut,
                             CString& interfaceVersionOut);
    LauncherUtils::ResponseError readOrganizationJSON(const CString& hash);
    LauncherUtils::ResponseError readConfigJSON(CString& version, CString& domain, 
                                                CString& content, bool& loggedIn, CString& organizationBuildTag);
    BOOL createConfigJSON();
    BOOL createApplicationRegistryKeys(int size);
    BOOL deleteApplicationRegistryKeys();
    BOOL createShortcuts();
    BOOL deleteShortcuts();
    HWND launchApplication();
    BOOL uninstallApplication();
    void tryToInstallLauncher(BOOL retry = FALSE);
    BOOL restartLauncher();

    //  getters
    const CString& getContentURL() const { return _contentURL; }
    const CString& getdomainURL() const { return _domainURL; }
    const CString& getVersion() const { return _version; }
    BOOL shouldShutDown() const { return _shouldShutdown; }
    BOOL shouldLaunch() const { return _shouldLaunch; }
    BOOL needsUpdate() const { return _shouldUpdate; }
    BOOL needsSelfUpdate() const { return _shouldUpdateLauncher; }
    BOOL needsSelfDownload() const { return _shouldDownloadLauncher; }
    BOOL needsUninstall() const { return _shouldUninstall; }
    BOOL needsInstall() const { return _shouldInstall; }
    BOOL needsToWait() const { return _shouldWait; }
    BOOL needsRestartNewLauncher() const { return _shouldRestartNewLauncher; }
    BOOL needsToSelfInstall() const { return _retryLauncherInstall; }
    BOOL willContinueUpdating() const { return _keepUpdating; }
    ContinueActionOnStart getContinueAction() { return _continueAction; }
    void setDisplayName(const CString& displayName) { _displayName = displayName; }
    bool isLoggedIn() const { return _loggedIn; }
    bool hasFailed() const { return _hasFailed; }
    void setFailed(bool hasFailed) { _hasFailed = hasFailed; }
    const CString& getLatestInterfaceURL() const { return _latestApplicationURL; }
    void uninstall() {
        _shouldUninstall = true;
        _shouldWait = false;
    };

    BOOL downloadFile(ProcessType type, const CString& url, CString& localPath);
    BOOL downloadContent();
    BOOL downloadApplication();
    BOOL downloadNewLauncher();
    BOOL installContent();
    BOOL extractApplication();
    void restartNewLauncher();
    void onZipExtracted(ProcessType type, int size);
    void onFileDownloaded(ProcessType type);
    float getProgress() const { return _progress; }
    void updateProgress(ProcessType processType, float progress);
    void onCancel();
    const CString& getLauncherVersion() const { return _launcherVersion; }
    CString getHttpUserAgent() const { return L"HQLauncher/" + _launcherVersion + L" (Windows)"; }

private:
    ProcessType _currentProcess { ProcessType::DownloadApplication };
    void onMostRecentBuildsReceived(const CString& response, LauncherUtils::ResponseError error);

    CString _latestApplicationURL;
    CString _latestVersion;
    CString _latestLauncherURL;
    CString _latestLauncherVersion;

    CString _contentURL;
    CString _domainURL;
    CString _version;
    CString _displayName;
    CString _tokensJSON;
    CString _applicationZipPath;
    CString _contentZipPath;
    CString _launcherVersion;
    CString _tempLauncherPath;
    CString _currentVersion;
    bool _isInstalled{ false };
    bool _loggedIn { false };
    bool _hasFailed { false };
    BOOL _shouldUpdate { FALSE };
    BOOL _shouldUninstall { FALSE };
    BOOL _shouldInstall { FALSE };
    BOOL _shouldShutdown { FALSE };
    BOOL _shouldLaunch { FALSE };
    BOOL _shouldWait { TRUE };
    BOOL _shouldUpdateLauncher { FALSE };
    BOOL _shouldDownloadLauncher { FALSE };
    BOOL _updateLauncherAllowed { TRUE };
    BOOL _shouldRestartNewLauncher { FALSE };
    BOOL _keepLoggingIn { FALSE };
    BOOL _keepUpdating { FALSE };
    BOOL _retryLauncherInstall { FALSE };
    ContinueActionOnStart _continueAction;
    float _progressOffset { 0.0f };
    float _progress { 0.0f };
    CStdioFile _logFile;
    Json::Value _latestBuilds;
    CString _defaultBuildTag;
    CString _organizationBuildTag;
};
