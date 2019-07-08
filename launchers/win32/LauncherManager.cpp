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
}

LauncherManager::~LauncherManager() {
}

void LauncherManager::init() {
    initLog();
    addToLog(_T("Getting most recent build"));
    LauncherUtils::ResponseError error = getMostRecentBuild(_latestApplicationURL, _latestVersion);
    if (error == LauncherUtils::ResponseError::NoError) {
        addToLog(_T("Latest version: ") + _latestVersion);
        CString currentVersion;
        if (isApplicationInstalled(currentVersion, _domainURL, _contentURL, _loggedIn) && _loggedIn) {
            addToLog(_T("Installed version: ") + currentVersion);
            if (_latestVersion.Compare(currentVersion) == 0) {
                addToLog(_T("Already running most recent build. Launching interface.exe"));
                _shouldLaunch = TRUE;
                _shouldShutdown = TRUE;
            } else {
                addToLog(_T("New build found. Updating"));
                _shouldUpdate = TRUE;
            }
        }
    } else {
        _hasFailed = true;
        CString msg;
        msg.Format(_T("Getting most recent build has failed with error: %d"), error);
        addToLog(msg);
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

BOOL LauncherManager::installLauncher() {
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
            addToLog(_T("Installing Launcher."));
            int launcherPID = -1;
            if (LauncherUtils::isProcessRunning(LAUNCHER_EXE_FILENAME, launcherPID)) {
                if (!LauncherUtils::shutdownProcess(launcherPID, 0)) {
                    addToLog(_T("Error shutting down the Launcher"));
                }
            }
            CopyFile(appPath, instalationPath, FALSE);
        }
    } else if (_shouldUninstall) {
        addToLog(_T("Launching Uninstall mode."));
        CString tempPath;
        if (getAndCreatePaths(PathType::Temp_Directory, tempPath)) {
            tempPath += _T("\\HQ_uninstaller_tmp.exe");
            CopyFile(instalationPath, tempPath, false);
            LauncherUtils::launchApplication(tempPath, _T(" --uninstall"));
            exit(0);
        }
    }
    return TRUE;
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
                                             CString& content, bool& loggedIn) {
    CString applicationDir;
    getAndCreatePaths(PathType::Launcher_Directory, applicationDir);
    CString applicationPath = applicationDir + "interface\\interface.exe";
    BOOL isApplicationInstalled = PathFileExistsW(applicationPath);
    BOOL configFileExist = PathFileExistsW(applicationDir + _T("interface\\config.json"));
    if (isApplicationInstalled && configFileExist) {
        LauncherUtils::ResponseError status = readConfigJSON(version, domain, content, loggedIn);
        return status == LauncherUtils::ResponseError::NoError;
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
    CString scriptsURL = installDir + _T("\\scripts\\simplifiedUI");
    CString scriptsParam = _T("--scripts \"") + scriptsURL + ("\" ");
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
    CString content;
    getAndCreatePaths(PathType::Content_Directory, content);
    config["content"] = LauncherUtils::cStringToStd(content);
    configFile << config;
    configFile.close();
    return TRUE;
}

LauncherUtils::ResponseError LauncherManager::readConfigJSON(CString& version, CString& domain, CString& content, bool& loggedIn) {
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
        configFile.close();
        return LauncherUtils::ResponseError::NoError;
    }
    configFile.close();
    return LauncherUtils::ResponseError::ParsingJSON;
}

LauncherUtils::ResponseError LauncherManager::readOrganizationJSON(const CString& hash) {
    CString contentTypeJson = L"content-type:application/json";
    CString response;
    CString url = _T("/organizations/") + hash + _T(".json");
    LauncherUtils::ResponseError error = LauncherUtils::makeHTTPCall(L"HQ Launcher", L"orgs.highfidelity.com", url,
        contentTypeJson, CStringA(), response, false);
    if (error != LauncherUtils::ResponseError::NoError) {
        return error;
    }
    Json::Value json;
    if (LauncherUtils::parseJSON(response, json)) {
        if (json["content_set_url"].isString() && json["domain"].isString()) {
            _contentURL = json["content_set_url"].asCString();
            _domainURL = json["domain"].asCString();
            return LauncherUtils::ResponseError::NoError;
        }
    }
    return LauncherUtils::ResponseError::ParsingJSON;
}

LauncherUtils::ResponseError LauncherManager::getMostRecentBuild(CString& urlOut, CString& versionOut) {
    CString contentTypeJson = L"content-type:application/json";
    CString response;
    LauncherUtils::ResponseError error = LauncherUtils::makeHTTPCall(L"HQ Launcher", L"thunder.highfidelity.com", L"/builds/api/tags/latest?format=json",
        contentTypeJson, CStringA(), response, false);
    if (error != LauncherUtils::ResponseError::NoError) {
        return error;
    }
    Json::Value json;
    if (LauncherUtils::parseJSON(response, json)) {
        int count = json["count"].isInt() ? json["count"].asInt() : 0;
        if (count > 0 && json["results"].isArray()) {
            for (int i = 0; i < count; i++) {
                if (json["results"][i].isObject()) {
                    Json::Value result = json["results"][i];
                    if (result["latest_version"].isInt()) {
                        std::string version = std::to_string(result["latest_version"].asInt());
                        versionOut = CString(version.c_str());
                    }
                    if (result["installers"].isObject() &&
                        result["installers"]["windows"].isObject() &&
                        result["installers"]["windows"]["zip_url"].isString()) {
                        urlOut = result["installers"]["windows"]["zip_url"].asCString();
                        return LauncherUtils::ResponseError::NoError;
                    }
                }
            }
        }
    }
    return LauncherUtils::ResponseError::ParsingJSON;
}

LauncherUtils::ResponseError LauncherManager::getAccessTokenForCredentials(const CString& username, const CString& password) {
    CStringA post = "grant_type=password&username=";
    post += username;
    post += "&password=";
    post += password;
    post += "&scope=owner";

    CString contentTypeText = L"content-type:application/x-www-form-urlencoded";
    CString response;
    LauncherUtils::ResponseError error = LauncherUtils::makeHTTPCall(L"HQ Launcher", L"metaverse.highfidelity.com", L"/oauth/token",
        contentTypeText, post, response, true);
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
    success = LauncherUtils::insertRegistryKey(REGISTRY_PATH, "Publisher", "High Fidelity");
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

void LauncherManager::onZipExtracted(ZipType type, int size) {
    if (type == ZipType::ZipContent) {
        addToLog(_T("Downloading application."));
        downloadApplication();
    } else if (type == ZipType::ZipApplication) {
        createShortcuts();
        CString versionPath;
        getAndCreatePaths(LauncherManager::PathType::Launcher_Directory, versionPath);
        addToLog(_T("Creating config.json"));
        createConfigJSON();
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
    BOOL success = LauncherUtils::unzipFileOnThread(ZipType::ZipApplication, LauncherUtils::cStringToStd(_applicationZipPath),
        LauncherUtils::cStringToStd(installPath), [&](int type, int size) {
        if (size > 0) {
            onZipExtracted((ZipType)type, size);
        } else {
            addToLog(_T("Error decompressing application zip file."));
            _hasFailed = true;
        }
    });
    if (success) {
        addToLog(_T("Created thread for unzipping application."));
    } else {
        addToLog(_T("Failed to create thread for unzipping application."));
    }
    return success;
}

void LauncherManager::onFileDownloaded(DownloadType type) {
    if (type == DownloadType::DownloadContent) {
        addToLog(_T("Installing content."));
        installContent();
    } else if (type == DownloadType::DownloadApplication) {
        addToLog(_T("Installing application."));
        extractApplication();
    }
}


BOOL LauncherManager::installContent() {
    std::string contentZipFile = LauncherUtils::cStringToStd(_contentZipPath);
    CString contentPath;
    getAndCreatePaths(LauncherManager::PathType::Content_Directory, contentPath);
    BOOL success = LauncherUtils::unzipFileOnThread(ZipType::ZipContent, contentZipFile,
        LauncherUtils::cStringToStd(contentPath), [&](int type, int size) {
        if (size > 0) {
            addToLog(_T("Content zip decompresed."));
            onZipExtracted((ZipType)type, size);
        } else {
            addToLog(_T("Error decompressing content zip file."));
            _hasFailed = true;
        }
    });
    if (success) {
        addToLog(_T("Created thread for unzipping content."));
    } else {
        addToLog(_T("Failed to create thread for unzipping content."));
    }
    return success;
}


BOOL LauncherManager::downloadFile(DownloadType type, const CString& url, CString& outPath) {
    CString fileName = url.Mid(url.ReverseFind('/') + 1);
    CString downloadDirectory;
    BOOL success = getAndCreatePaths(LauncherManager::PathType::Download_Directory, downloadDirectory);
    outPath = downloadDirectory + fileName;
    if (success) {
        addToLog(_T("Downloading: ") + url);
        if (!LauncherUtils::downloadFileOnThread(type, url, outPath, [&](int type, bool error) {
            if (!error) {
                onFileDownloaded((DownloadType)type);
            } else {
                if (type == DownloadType::DownloadApplication) {
                    addToLog(_T("Error downloading content."));
                } else {
                    addToLog(_T("Error downloading application."));
                }
                _hasFailed = true;
            }
        })) {
            success = FALSE;
        }
    }
    return success;
}

BOOL LauncherManager::downloadContent() {
    addToLog(_T("Downloading content."));
    CString contentURL = getContentURL();
    return downloadFile(DownloadType::DownloadContent, contentURL, _contentZipPath);
}

BOOL LauncherManager::downloadApplication() {
    CString applicationURL = getLatestInterfaceURL();
    return downloadFile(DownloadType::DownloadApplication, applicationURL, _applicationZipPath);
}
