//
//  LauncherManager.cpp
//
//  Created by Luis Cuenca on 6/5/2019.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "stdafx.h"
#include <time.h>
#include <fstream>

#include "LauncherManager.h"


LauncherManager::LauncherManager() {
    int tokenPos = 0;
    _launcherVersion = CString(LAUNCHER_BUILD_VERSION).Tokenize(_T("-"), tokenPos);
}

LauncherManager::~LauncherManager() {
}

void LauncherManager::init(BOOL allowUpdate, ContinueActionOnStart continueAction) {
    initLog();
    _updateLauncherAllowed = allowUpdate;
    _continueAction = continueAction;
    CString msg;
    msg.Format(_T("Start Screen: %s"), getContinueActionParam(continueAction));
    addToLog(msg);
    _shouldWait = _continueAction == ContinueActionOnStart::ContinueNone;
    if (_continueAction == ContinueActionOnStart::ContinueUpdate) {
        _progressOffset = CONTINUE_UPDATING_GLOBAL_OFFSET;
    }
    addToLog(_T("Launcher is running version: " + _launcherVersion));
    addToLog(_T("Getting most recent builds"));
    _isInstalled = isApplicationInstalled(_currentVersion, _domainURL, _contentURL, _loggedIn, _organizationBuildTag);
    getMostRecentBuilds(_latestLauncherURL, _latestLauncherVersion, _latestApplicationURL, _latestVersion);
}

CString LauncherManager::getContinueActionParam(LauncherManager::ContinueActionOnStart continueAction) {
    switch (continueAction) {
        case LauncherManager::ContinueActionOnStart::ContinueNone:
            return _T("");
        case LauncherManager::ContinueActionOnStart::ContinueLogIn:
            return _T("LogIn");
        case LauncherManager::ContinueActionOnStart::ContinueUpdate:
            return _T("Update");
        case LauncherManager::ContinueActionOnStart::ContinueFinish:
            return _T("Finish");
        default:
            return _T("");
    }
}

LauncherManager::ContinueActionOnStart LauncherManager::getContinueActionFromParam(const CString& param) {
    if (param.Compare(_T("LogIn")) == 0) {
        return ContinueActionOnStart::ContinueLogIn;
    } else if (param.Compare(_T("Update")) == 0) {
        return ContinueActionOnStart::ContinueUpdate;
    } else if (param.Compare(_T("Finish")) == 0) {
        return ContinueActionOnStart::ContinueFinish;
    } else {
        return ContinueActionOnStart::ContinueNone;
    }
}
BOOL LauncherManager::initLog() {
    CString logPath;
    auto result = getAndCreatePaths(PathType::Launcher_Directory, logPath);
    if (result) {
        logPath += _T("log.txt");
        return result = _logFile.Open(logPath, CFile::modeCreate | CFile::modeReadWrite);
    }
    return FALSE;
}

BOOL LauncherManager::addToLog(const CString& line) {
    if (_logFile.m_hFile != CStdioFile::hFileNull) {
        char buff[100];
        time_t now = time(0);
        tm ltm;
        localtime_s(&ltm, &now);

        strftime(buff, 100, "%Y-%m-%d %H:%M:%S", &ltm);
        CString timeStr = CString(buff);
        _logFile.WriteString(timeStr + _T("  ") + line + _T("\n"));
        return TRUE;
    }
    return FALSE;
}

void LauncherManager::closeLog() {
    if (_logFile.m_hFile != CStdioFile::hFileNull) {
        _logFile.Close();
    }
}

void LauncherManager::saveErrorLog() {
    CString logPath = _logFile.GetFilePath();
    CString errorLogPath;
    auto result = getAndCreatePaths(PathType::Launcher_Directory, errorLogPath);
    if (result) {
        CString filename;
        errorLogPath += _T("log_error.txt");
        closeLog();
        CopyFile(logPath, errorLogPath, FALSE);
    }
}

void LauncherManager::tryToInstallLauncher(BOOL retry) {
    CString appPath;
    BOOL result = getAndCreatePaths(PathType::Running_Path, appPath);
    if (!result) {
        MessageBox(NULL, L"Error getting app directory", L"Path Error", MB_OK | MB_ICONERROR);
    }
    CString installDirectory;
    CString appDirectory = appPath.Left(appPath.ReverseFind('\\') + 1);
    result = getAndCreatePaths(PathType::Launcher_Directory, installDirectory);
    if (!result) {
        MessageBox(NULL, L"Error getting app desired directory", L"Path Error", MB_OK | MB_ICONERROR);
    }

    CString instalationPath = installDirectory + LAUNCHER_EXE_FILENAME;
    if (!installDirectory.Compare(appDirectory) == 0) {
        if (!_shouldUninstall) {
            // The installer is not running on the desired location and has to be installed
            // Kill of running before self-copy
            addToLog(_T("Trying to install launcher."));
            int launcherPID = -1;
            if (LauncherUtils::isProcessRunning(LAUNCHER_EXE_FILENAME, launcherPID)) {
                if (!LauncherUtils::shutdownProcess(launcherPID, 0)) {
                    addToLog(_T("Error shutting down the Launcher"));
                }
            }
            const int LAUNCHER_INSTALL_RETRYS = 10;
            const int WAIT_BETWEEN_RETRYS_MS = 10;
            int installTrys = retry ? LAUNCHER_INSTALL_RETRYS : 0;
            for (int i = 0; i <= installTrys; i++) {
                _retryLauncherInstall = !CopyFile(appPath, instalationPath, FALSE);
                if (!_retryLauncherInstall) {
                    addToLog(_T("Launcher installed successfully."));
                    break;
                } else if (i < installTrys) {
                    CString msg;
                    msg.Format(_T("Installing launcher try: %d"), i);
                    addToLog(msg);
                    Sleep(WAIT_BETWEEN_RETRYS_MS);
                } else if (installTrys > 0) {
                    addToLog(_T("Error installing launcher."));
                    _retryLauncherInstall = false;
                    _hasFailed = true;
                } else {
                    addToLog(_T("Old launcher is still running. Install could not be completed."));
                }
            }
        }
    } else if (_shouldUninstall) {
        addToLog(_T("Launching Uninstall mode."));
        CString tempPath;
        if (getAndCreatePaths(PathType::Temp_Directory, tempPath)) {
            tempPath += _T("\\HQ_uninstaller_tmp.exe");
            if (!CopyFile(instalationPath, tempPath, false)) {
                addToLog(_T("Error copying uninstaller to tmp directory."));
                _hasFailed = true;
            } else {
                LauncherUtils::launchApplication(tempPath, _T(" --uninstall"));
                exit(0);
            }
        }
    }
}

BOOL LauncherManager::restartLauncher() {
    addToLog(_T("Restarting Launcher."));
    CString installDirectory;
    if (getAndCreatePaths(PathType::Launcher_Directory, installDirectory)) {
        CString installPath = installDirectory + LAUNCHER_EXE_FILENAME;
        LauncherUtils::launchApplication(installPath, _T(" --restart"));
        exit(0);
    }
    addToLog(_T("Error restarting Launcher."));
    return FALSE;
}

void LauncherManager::updateProgress(ProcessType processType, float progress) {
    switch (processType) {
    case ProcessType::DownloadLauncher:
        break;
    case ProcessType::Uninstall:
        _progress = progress;
        break;
    case ProcessType::DownloadContent:
        _progress = DOWNLOAD_CONTENT_INSTALL_WEIGHT * progress;
        break;
    case ProcessType::UnzipContent:
        _progress = DOWNLOAD_CONTENT_INSTALL_WEIGHT +
                    EXTRACT_CONTENT_INSTALL_WEIGHT * progress;
        break;
    case ProcessType::DownloadApplication:
        _progress = !_shouldUpdate ?
                    (DOWNLOAD_CONTENT_INSTALL_WEIGHT +
                    EXTRACT_CONTENT_INSTALL_WEIGHT +
                    DOWNLOAD_APPLICATION_INSTALL_WEIGHT * progress) :
                    DOWNLOAD_APPLICATION_UPDATE_WEIGHT * progress;
        break;
    case ProcessType::UnzipApplication:
        _progress = !_shouldUpdate ?
                    (DOWNLOAD_CONTENT_INSTALL_WEIGHT +
                    EXTRACT_CONTENT_INSTALL_WEIGHT +
                    DOWNLOAD_APPLICATION_INSTALL_WEIGHT +
                    EXTRACT_APPLICATION_INSTALL_WEIGHT * progress) :
                    (DOWNLOAD_APPLICATION_UPDATE_WEIGHT +
                     EXTRACT_APPLICATION_UPDATE_WEIGHT * progress);
        break;
    default:
        break;
    }
    _progress = _progressOffset + (1.0f - _progressOffset) * _progress;
    TRACE("progress = %f\n", _progress);
}

BOOL LauncherManager::createShortcuts() {
    CString desktopLnkPath;
    addToLog(_T("Creating shortcuts."));
    getAndCreatePaths(PathType::Desktop_Directory, desktopLnkPath);
    desktopLnkPath += _T("\\HQ Launcher.lnk");
    CString installDir;
    getAndCreatePaths(PathType::Launcher_Directory, installDir);
    CString installPath = installDir + LAUNCHER_EXE_FILENAME;
    if (!LauncherUtils::createLink(installPath, (LPCSTR)CStringA(desktopLnkPath), _T("CLick to Setup and Launch HQ."))) {
        return FALSE;
    }
    CString startLinkPath;
    getAndCreatePaths(PathType::StartMenu_Directory, startLinkPath);
    CString appStartLinkPath = startLinkPath + _T("HQ Launcher.lnk");
    CString uniStartLinkPath = startLinkPath + _T("Uninstall HQ.lnk");
    CString uniLinkPath = installDir + _T("Uninstall HQ.lnk");
    if (!LauncherUtils::createLink(installPath, (LPCSTR)CStringA(appStartLinkPath), _T("CLick to Setup and Launch HQ."))) {
        return FALSE;
    }
    if (!LauncherUtils::createLink(installPath, (LPCSTR)CStringA(uniStartLinkPath), _T("CLick to Uninstall HQ."), _T("--uninstall"))) {
        return FALSE;
    }
    if (!LauncherUtils::createLink(installPath, (LPCSTR)CStringA(uniLinkPath), _T("CLick to Uninstall HQ."), _T("--uninstall"))) {
        return FALSE;
    }
    return TRUE;
}

BOOL LauncherManager::deleteShortcuts() {
    CString desktopLnkPath;
    addToLog(_T("Deleting shortcuts."));
    getAndCreatePaths(PathType::Desktop_Directory, desktopLnkPath);
    desktopLnkPath += _T("\\HQ Launcher.lnk");
    BOOL success = LauncherUtils::deleteFileOrDirectory(desktopLnkPath);
    CString startLinkPath;
    getAndCreatePaths(PathType::StartMenu_Directory, startLinkPath);
    return success && LauncherUtils::deleteFileOrDirectory(startLinkPath);
}

BOOL LauncherManager::isApplicationInstalled(CString& version, CString& domain,
                                             CString& content, bool& loggedIn, CString& organizationBuildTag) {
    CString applicationDir;
    getAndCreatePaths(PathType::Launcher_Directory, applicationDir);
    CString applicationPath = applicationDir + "interface\\interface.exe";
    BOOL isInstalled = PathFileExistsW(applicationPath);
    BOOL configFileExist = PathFileExistsW(applicationDir + _T("interface\\config.json"));
    if (configFileExist) {
        LauncherUtils::ResponseError status = readConfigJSON(version, domain, content, loggedIn, organizationBuildTag);
        return isInstalled && status == LauncherUtils::ResponseError::NoError;
    }
    return FALSE;
}

BOOL LauncherManager::getAndCreatePaths(PathType type, CString& outPath) {

    if (type == PathType::Running_Path) {
        char appPath[MAX_PATH];
        DWORD size = GetModuleFileNameA(NULL, appPath, MAX_PATH);
        if (size) {
            outPath = CString(appPath);
            return TRUE;
        }
    } else if (type == PathType::Desktop_Directory) {
        TCHAR desktopPath[MAX_PATH];
        auto success = SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath));
        outPath = CString(desktopPath);
        return success;
    } else if (type == PathType::Temp_Directory) {
        TCHAR tempPath[MAX_PATH];
        auto success = GetTempPath(MAX_PATH, tempPath);
        outPath = CString(tempPath);
        return success;
    } else {
        TCHAR localDataPath[MAX_PATH];
        if (type == PathType::StartMenu_Directory) {
            TCHAR startMenuPath[MAX_PATH];
            auto success = SUCCEEDED(SHGetFolderPath(NULL, CSIDL_STARTMENU, NULL, 0, startMenuPath));
            outPath = CString(startMenuPath) + _T("\\Programs\\HQ\\");
            success = SHCreateDirectoryEx(NULL, outPath, NULL) || ERROR_ALREADY_EXISTS == GetLastError();
            return success;
        } else if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localDataPath))) {
            _tcscat_s(localDataPath, _T("\\") + DIRECTORY_NAME_APP + _T("\\"));
            outPath = CString(localDataPath);
            if (type == PathType::Download_Directory) {
                outPath += DIRECTORY_NAME_DOWNLOADS + _T("\\");
            } else if (type == PathType::Interface_Directory) {
                outPath += DIRECTORY_NAME_INTERFACE;
            } else if (type == PathType::Content_Directory) {
                outPath += DIRECTORY_NAME_CONTENT;
            }
            return (CreateDirectory(outPath, NULL) || ERROR_ALREADY_EXISTS == GetLastError());
        }
    }
    return FALSE;
}

BOOL LauncherManager::getInstalledVersion(const CString& path, CString& version) {
    CStdioFile cfile;
    BOOL success = cfile.Open(path, CFile::modeRead);
    if (success) {
        cfile.ReadString(version);
        cfile.Close();
    }
    return success;
}

HWND LauncherManager::launchApplication() {
    CString installDir;
    LauncherManager::getAndCreatePaths(PathType::Interface_Directory, installDir);
    CString interfaceExe = installDir + _T("\\interface.exe");
    CString urlParam = _T("--url \"") + _domainURL + ("\" ");
    CString scriptsURL = installDir + _T("\\scripts\\simplifiedUIBootstrapper.js");
    scriptsURL.Replace(_T("\\"), _T("/"));
    CString scriptsParam = _T("--defaultScriptsOverride \"") + scriptsURL + ("\" ");
    CString cacheDir;
    LauncherManager::getAndCreatePaths(PathType::Content_Directory, cacheDir);
    CString cacheParam = _T("--cache \"") + cacheDir + ("\" ");
    CString nameParam = !_displayName.IsEmpty() ? _T("--displayName \"") + _displayName + ("\" ") : _T("");
    CString tokensParam = _T("");
    if (!_tokensJSON.IsEmpty()) {
        CString parsedTokens = _tokensJSON;
        parsedTokens.Replace(_T("\""), _T("\\\""));
        tokensParam = _T("--tokens \"");
        tokensParam += parsedTokens + _T("\" ");
    }
    CString bookmarkParam = _T("--setBookmark hqhome=\"") + _domainURL + ("\" ");
    CString params = urlParam + scriptsParam + cacheParam + nameParam + tokensParam + bookmarkParam + EXTRA_PARAMETERS;
    _shouldLaunch = FALSE;
    return LauncherUtils::executeOnForeground(interfaceExe, params);
}

BOOL LauncherManager::createConfigJSON() {
    CString configPath;
    LauncherManager::getAndCreatePaths(PathType::Interface_Directory, configPath);
    configPath += "\\config.json";
    std::ofstream configFile(configPath, std::ofstream::binary);
    if (configFile.fail()) {
        return FALSE;
    }
    Json::Value config;
    CString applicationPath;
    LauncherManager::getAndCreatePaths(PathType::Launcher_Directory, applicationPath);
    applicationPath += LAUNCHER_EXE_FILENAME;
    config["loggedIn"] = _loggedIn;
    config["launcherPath"] = LauncherUtils::cStringToStd(applicationPath);
    config["version"] = LauncherUtils::cStringToStd(_latestVersion);
    config["domain"] = LauncherUtils::cStringToStd(_domainURL);
    config["organizationBuildTag"] = LauncherUtils::cStringToStd(_organizationBuildTag);
    CString content;
    getAndCreatePaths(PathType::Content_Directory, content);
    config["content"] = LauncherUtils::cStringToStd(content);
    configFile << config;
    configFile.close();
    return TRUE;
}

LauncherUtils::ResponseError LauncherManager::readConfigJSON(CString& version, CString& domain,
                                                             CString& content, bool& loggedIn, CString& organizationBuildTag) {
    CString configPath;
    getAndCreatePaths(PathType::Interface_Directory, configPath);
    configPath += "\\config.json";
    std::ifstream configFile(configPath, std::ifstream::binary);
    if (configFile.fail()) {
        return LauncherUtils::ResponseError::Open;
    }
    Json::Value config;
    configFile >> config;
    if (config["version"].isString() &&
        config["domain"].isString() &&
        config["content"].isString()) {
        loggedIn = config["loggedIn"].asBool();
        version = config["version"].asCString();
        domain = config["domain"].asCString();
        content = config["content"].asCString();

        if (config["organizationBuildTag"].isString()) {
            organizationBuildTag = config["organizationBuildTag"].asCString();
        } else {
            organizationBuildTag = "";
        }

        configFile.close();
        return LauncherUtils::ResponseError::NoError;
    }
    configFile.close();
    return LauncherUtils::ResponseError::ParsingJSON;
}

bool findBuildInResponse(const Json::Value& json, const CString& tag, CString& interfaceUrlOut, CString& interfaceVersionOut) {
    if (json["results"].isArray()) {
        auto& results = json["results"];
        int count = results.size();
        for (int i = 0; i < count; i++) {
            if (results[i].isObject()) {
                Json::Value result = results[i];
                if (result["name"].asCString() == tag) {
                    if (result["latest_version"].isInt()) {
                        std::string version = std::to_string(result["latest_version"].asInt());
                        interfaceVersionOut = CString(version.c_str());
                    } else {
                        return false;
                    }
                    if (result["installers"].isObject() &&
                        result["installers"]["windows"].isObject() &&
                        result["installers"]["windows"]["zip_url"].isString()) {
                        interfaceUrlOut = result["installers"]["windows"]["zip_url"].asCString();
                    } else {
                        return false;
                    }
                    return true;
                }
            }
        }
    }

    return false;
}


LauncherUtils::ResponseError LauncherManager::readOrganizationJSON(const CString& hash) {
    CString contentTypeJson = L"content-type:application/json";
    CString response;
    CString url = _T("/organizations/") + hash + _T(".json");
    LauncherUtils::ResponseError error = LauncherUtils::makeHTTPCall(getHttpUserAgent(),
                                                                     true, L"orgs.highfidelity.com", url,
                                                                     contentTypeJson, CStringA(),
                                                                     response, false);
    if (error != LauncherUtils::ResponseError::NoError) {
        return error;
    }
    Json::Value json;
    if (LauncherUtils::parseJSON(response, json)) {
        if (json["content_set_url"].isString() && json["domain"].isString()) {
            _contentURL = json["content_set_url"].asCString();
            _domainURL = json["domain"].asCString();
            _organizationBuildTag = json.get("build_tag", "").asCString();
            auto buildTag = _organizationBuildTag.IsEmpty() ? _defaultBuildTag : _organizationBuildTag;

            if (!findBuildInResponse(_latestBuilds, buildTag, _latestApplicationURL, _latestVersion)) {
                return LauncherUtils::ResponseError::ParsingJSON;
            }

            return LauncherUtils::ResponseError::NoError;
        }
    }
    return LauncherUtils::ResponseError::ParsingJSON;
}

void LauncherManager::getMostRecentBuilds(CString& launcherUrlOut, CString& launcherVersionOut,
                                          CString& interfaceUrlOut, CString& interfaceVersionOut) {
    CString contentTypeJson = L"content-type:application/json";
    std::function<void(CString, int)> httpCallback = [&](CString response, int err) {
        LauncherUtils::ResponseError error = LauncherUtils::ResponseError(err);
        if (error == LauncherUtils::ResponseError::NoError) {
            Json::Value& json = _latestBuilds;
            if (LauncherUtils::parseJSON(response, json)) {
                _defaultBuildTag = json.get("default_tag", "").asCString();
                auto buildTag = _organizationBuildTag.IsEmpty() ? _defaultBuildTag : _organizationBuildTag;
                addToLog(_T("Build tag is: ") + buildTag);

                if (json["launcher"].isObject()) {
                    if (json["launcher"]["windows"].isObject() && json["launcher"]["windows"]["url"].isString()) {
                        launcherUrlOut = json["launcher"]["windows"]["url"].asCString();
                    }
                    if (json["launcher"]["version"].isInt()) {
                        std::string version = std::to_string(json["launcher"]["version"].asInt());
                        launcherVersionOut = CString(version.c_str());
                    }
                }

                if (launcherUrlOut.IsEmpty() || launcherVersionOut.IsEmpty()) {
                    error = LauncherUtils::ResponseError::ParsingJSON;
                }

                if (!findBuildInResponse(json, buildTag, _latestApplicationURL, _latestVersion)) {
                    addToLog(_T("Failed to find build"));
                    error = LauncherUtils::ResponseError::ParsingJSON;
                }
            }
        }
        onMostRecentBuildsReceived(response, error);
    };

    bool useHTTPS{ true };

    CString domainName;
    if (domainName.GetEnvironmentVariable(L"HQ_LAUNCHER_BUILDS_DOMAIN")) {
        addToLog(_T("Using overridden builds domain: ") + domainName);
        useHTTPS = false;
    } else {
        // We have 2 ways to adjust the domain, one of which forces http (HQ_LAUNCHER_BUILDS_DOMAIN),
        // the other (HIFI_THUNDER_URL) of which uses https. They represent different use-cases, but
        // would ideally be combined by including the protocol in the url, but because
        // code is going to be replaced soon, we will leave it like this for now.
        if (domainName.GetEnvironmentVariable(L"HIFI_THUNDER_URL")) {
            addToLog(_T("Using overridden thunder url: ") + domainName);
        } else {
            domainName = L"thunder.highfidelity.com";
        }
    }

    CString pathName;
    if (pathName.GetEnvironmentVariable(L"HQ_LAUNCHER_BUILDS_PATH")) {
        addToLog(_T("Using overridden builds path: ") + pathName);
        useHTTPS = false;
    } else {
        pathName = L"/builds/api/tags/latest?format=json";
    }

    LauncherUtils::httpCallOnThread(getHttpUserAgent(),
                                    useHTTPS,
                                    domainName,
                                    pathName,
                                    contentTypeJson, CStringA(), false, httpCallback);
}

void LauncherManager::onMostRecentBuildsReceived(const CString& response, LauncherUtils::ResponseError error) {
    if (error == LauncherUtils::ResponseError::NoError) {
        addToLog(_T("Latest launcher version: ") + _latestLauncherVersion);
        CString currentVersion = _currentVersion;
        BOOL isInstalled = _isInstalled && _loggedIn;
        bool newInterfaceVersion = _latestVersion.Compare(currentVersion) != 0;
        bool newLauncherVersion = _latestLauncherVersion.Compare(_launcherVersion) != 0 && _updateLauncherAllowed;
        if (newLauncherVersion) {
            CString updatingMsg;
            updatingMsg.Format(_T("Updating Launcher from version: %s to version: %s"), _launcherVersion, _latestLauncherVersion);
            addToLog(updatingMsg);
            _shouldUpdateLauncher = TRUE;
            _shouldDownloadLauncher = TRUE;
            _keepLoggingIn = !isInstalled;
            _keepUpdating = isInstalled && newInterfaceVersion;
        } else {
            if (_updateLauncherAllowed) {
                addToLog(_T("Already running most recent build. Launching interface.exe"));
            } else {
                addToLog(_T("Updating the launcher was not allowed --noUpdate"));
            }
            if (isInstalled) {
                addToLog(_T("Installed version: ") + currentVersion);
                if (!newInterfaceVersion) {
                    addToLog(_T("Already running most recent build. Launching interface.exe"));
                    _shouldLaunch = TRUE;
                    _shouldShutdown = TRUE;
                } else {
                    addToLog(_T("New build found. Updating"));
                    _shouldUpdate = TRUE;
                }
            } else if (_loggedIn) {
                addToLog(_T("Interface not found but logged in. Reinstalling"));
                _shouldUpdate = TRUE;
            } else {
                _shouldInstall = TRUE;
            }
        }
        _shouldWait = FALSE;

    } else {
        setFailed(true);
        CString msg;
        msg.Format(_T("Getting most recent builds has failed with error: %d"), error);
        addToLog(msg);
        msg.Format(_T("Response: %s"), response);
        addToLog(msg);
    }
}

LauncherUtils::ResponseError LauncherManager::getAccessTokenForCredentials(const CString& username,
                                                                           const CString& password) {
    CStringA post = "grant_type=password&username=";
    post += username;
    post += "&password=";
    post += password;
    post += "&scope=owner";

    CString contentTypeText = L"content-type:application/x-www-form-urlencoded";
    CString response;
    LauncherUtils::ResponseError error = LauncherUtils::makeHTTPCall(getHttpUserAgent(),
                                                                     true,
                                                                     L"metaverse.highfidelity.com",
                                                                     L"/oauth/token",
                                                                     contentTypeText, post,
                                                                     response, true);
    if (error != LauncherUtils::ResponseError::NoError) {
        return error;
    }
    Json::Value json;
    if (LauncherUtils::parseJSON(response, json)) {
        if (json["error"].isString()) {
            return LauncherUtils::ResponseError::BadCredentials;
        } else if (json["access_token"].isString()) {
            _tokensJSON = response;
            return LauncherUtils::ResponseError::NoError;
        }
    }
    return LauncherUtils::ResponseError::ParsingJSON;
}

BOOL LauncherManager::createApplicationRegistryKeys(int size) {
    const std::string REGISTRY_PATH = "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\HQ";
    BOOL success = LauncherUtils::insertRegistryKey(REGISTRY_PATH, "DisplayName", "HQ");
    CString installDir;
    getAndCreatePaths(PathType::Launcher_Directory, installDir);
    success = LauncherUtils::insertRegistryKey(REGISTRY_PATH, "InstallLocation", LauncherUtils::cStringToStd(installDir));
    std::string applicationExe = LauncherUtils::cStringToStd(installDir + LAUNCHER_EXE_FILENAME);
    std::string uninstallPath = '"' + applicationExe + '"' + " --uninstall";
    success = LauncherUtils::insertRegistryKey(REGISTRY_PATH, "UninstallString", uninstallPath);
    success = LauncherUtils::insertRegistryKey(REGISTRY_PATH, "DisplayVersion", LauncherUtils::cStringToStd(_latestVersion));
    success = LauncherUtils::insertRegistryKey(REGISTRY_PATH, "DisplayIcon", applicationExe);
    success = LauncherUtils::insertRegistryKey(REGISTRY_PATH, "Publisher", "Vircadia");
    success = LauncherUtils::insertRegistryKey(REGISTRY_PATH, "InstallDate", LauncherUtils::cStringToStd(CTime::GetCurrentTime().Format("%Y%m%d")));
    success = LauncherUtils::insertRegistryKey(REGISTRY_PATH, "EstimatedSize", (DWORD)size);
    success = LauncherUtils::insertRegistryKey(REGISTRY_PATH, "NoModify", (DWORD)1);
    success = LauncherUtils::insertRegistryKey(REGISTRY_PATH, "NoRepair", (DWORD)1);
    return success;
}

BOOL LauncherManager::deleteApplicationRegistryKeys() {
    const CString REGISTRY_PATH = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\HQ");
    return LauncherUtils::deleteRegistryKey(REGISTRY_PATH);
}

BOOL LauncherManager::uninstallApplication() {
    CString installDir;
    getAndCreatePaths(PathType::Launcher_Directory, installDir);
    BOOL success = LauncherUtils::deleteFileOrDirectory(installDir);
    if (success) {
        deleteShortcuts();
        deleteApplicationRegistryKeys();
    }
    return success;
}

void LauncherManager::onZipExtracted(ProcessType type, int size) {
    if (type == ProcessType::UnzipContent) {
        addToLog(_T("Downloading application."));
        downloadApplication();
    } else if (type == ProcessType::UnzipApplication) {
        createShortcuts();
        addToLog(_T("Launching application."));
        _shouldLaunch = TRUE;
        if (!_shouldUpdate) {
            addToLog(_T("Creating registry keys."));
            createApplicationRegistryKeys(size);
        }
        _shouldShutdown = TRUE;
    }
}

BOOL LauncherManager::extractApplication() {
    CString installPath;
    getAndCreatePaths(LauncherManager::PathType::Interface_Directory, installPath);
    std::function<void(int, int)> onExtractFinished = [&](int type, int size) {
        addToLog(_T("Creating config.json"));
        createConfigJSON();
        if (size > 0) {
            onZipExtracted((ProcessType)type, size);
        } else {
            addToLog(_T("Error decompressing application zip file."));
            setFailed(true);
        }
    };
    std::function<void(float)> onProgress = [&](float progress) {
        updateProgress(ProcessType::UnzipApplication, max(progress, 0.0f));
    };
    _currentProcess = ProcessType::UnzipApplication;
    BOOL success = LauncherUtils::unzipFileOnThread(ProcessType::UnzipApplication,
                                                    LauncherUtils::cStringToStd(_applicationZipPath),
                                                    LauncherUtils::cStringToStd(installPath),
                                                    onExtractFinished, onProgress);
    if (success) {
        addToLog(_T("Created thread for unzipping application."));
    } else {
        addToLog(_T("Failed to create thread for unzipping application."));
    }
    return success;
}

void LauncherManager::onFileDownloaded(ProcessType type) {
    if (type == ProcessType::DownloadContent) {
        addToLog(_T("Deleting content directory before install"));
        CString contentDir;
        getAndCreatePaths(PathType::Content_Directory, contentDir);
        LauncherUtils::deleteDirectoryOnThread(contentDir, [&](bool error) {
            if (!error) {
                addToLog(_T("Installing content."));
                installContent();
            } else {
                addToLog(_T("Error deleting content directory."));
                setFailed(true);
            }
        });
    } else if (type == ProcessType::DownloadApplication) {
        addToLog(_T("Deleting application directory before install"));
        CString applicationDir;
        getAndCreatePaths(PathType::Interface_Directory, applicationDir);
        LauncherUtils::deleteDirectoryOnThread(applicationDir, [&](bool error) {
            if (!error) {
                addToLog(_T("Installing application."));
                extractApplication();
            } else {
                addToLog(_T("Error deleting install directory."));
                setFailed(true);
            }
        });
    } else if (type == ProcessType::DownloadLauncher) {
        _shouldRestartNewLauncher = true;
    }
}

void LauncherManager::restartNewLauncher() {
    closeLog();
    ContinueActionOnStart continueAction = ContinueActionOnStart::ContinueFinish;
    if (_keepUpdating) {
        continueAction = ContinueActionOnStart::ContinueUpdate;
    } else if (_keepLoggingIn) {
        continueAction = ContinueActionOnStart::ContinueLogIn;
    }
    CStringW params;
    params.Format(_T(" --restart --noUpdate --continueAction %s"), getContinueActionParam(continueAction));
    LauncherUtils::launchApplication(_tempLauncherPath, params.GetBuffer());
    Sleep(500);
}


BOOL LauncherManager::installContent() {
    std::string contentZipFile = LauncherUtils::cStringToStd(_contentZipPath);
    CString contentPath;
    getAndCreatePaths(LauncherManager::PathType::Content_Directory, contentPath);
    std::function<void(int, int)> onInstallFinished = [&](int type, int size) {
        if (size > 0) {
            addToLog(_T("Content zip decompresed."));
            onZipExtracted((ProcessType)type, size);
        }
        else {
            addToLog(_T("Error decompressing content zip file."));
            setFailed(true);
        }
    };
    std::function<void(float)> onProgress = [&](float progress) {
        updateProgress(ProcessType::UnzipContent, max(progress, 0.0f));
    };
    _currentProcess = ProcessType::UnzipContent;
    BOOL success = LauncherUtils::unzipFileOnThread(ProcessType::UnzipContent, contentZipFile,
        LauncherUtils::cStringToStd(contentPath), onInstallFinished, onProgress);
    if (success) {
        addToLog(_T("Created thread for unzipping content."));
    } else {
        addToLog(_T("Failed to create thread for unzipping content."));
    }
    return success;
}


BOOL LauncherManager::downloadFile(ProcessType type, const CString& url, CString& outPath) {
    BOOL success = TRUE;
    if (outPath.IsEmpty()) {
        CString fileName = url.Mid(url.ReverseFind('/') + 1);
        CString downloadDirectory;
        BOOL success = getAndCreatePaths(LauncherManager::PathType::Download_Directory, downloadDirectory);
        outPath = downloadDirectory + fileName;
    }
    _currentProcess = type;
    if (success) {
        addToLog(_T("Downloading: ") + url);
        std::function<void(int, bool)> onDownloadFinished = [&](int type, bool error) {
            if (!error) {
                onFileDownloaded((ProcessType)type);
            } else {
                if (type == ProcessType::DownloadApplication) {
                    addToLog(_T("Error downloading content."));
                } else if (type == ProcessType::DownloadLauncher) {
                    addToLog(_T("Error downloading launcher."));
                } else {
                    addToLog(_T("Error downloading application."));
                }
                setFailed(true);
            }
        };
        std::function<void(float)> onProgress = [&, type](float progress) {
            updateProgress(_currentProcess, max(progress, 0.0f));
        };
        success = LauncherUtils::downloadFileOnThread(type, url, outPath, onDownloadFinished, onProgress);
    }
    return success;
}

BOOL LauncherManager::downloadContent() {
    addToLog(_T("Downloading content."));
    CString contentURL = getContentURL();
    return downloadFile(ProcessType::DownloadContent, contentURL, _contentZipPath);
}

BOOL LauncherManager::downloadApplication() {
    CString applicationURL = getLatestInterfaceURL();
    return downloadFile(ProcessType::DownloadApplication, applicationURL, _applicationZipPath);
}

BOOL LauncherManager::downloadNewLauncher() {
    _shouldDownloadLauncher = FALSE;
    getAndCreatePaths(PathType::Temp_Directory, _tempLauncherPath);
    CString tempName = _T("HQLauncher") + _launcherVersion + _T(".exe");
    _tempLauncherPath += tempName;
    return downloadFile(ProcessType::DownloadLauncher, _latestLauncherURL, _tempLauncherPath);
}

void LauncherManager::onCancel() {
    if (_currentProcess == ProcessType::UnzipApplication) {
        _latestVersion = _T("");
        _version = _T("");
        createConfigJSON();
    }
}
