//
//  GlobalData.cpp
//  StackManagerQt/src
//
//  Created by Mohammed Nafees on 6/25/14.
//  Copyright (c) 2014 High Fidelity. All rights reserved.
//

#include "GlobalData.h"
#include "StackManagerVersion.h"

#include <QMutex>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

GlobalData& GlobalData::getInstance() {
    static GlobalData staticInstance;
    return staticInstance;
}

GlobalData::GlobalData() {
    QString urlBase = URL_BASE;
#if defined Q_OS_OSX
    _platform = "mac";
#elif defined Q_OS_WIN32
    _platform = "win";
#elif defined Q_OS_LINUX
    _platform = "linux";
#endif

    _resourcePath = "resources/";
    _assignmentClientExecutable = "assignment-client";
    _domainServerExecutable = "domain-server";
	QString applicationSupportDirectory = QCoreApplication::applicationDirPath();

    _clientsLaunchPath = QDir::toNativeSeparators(applicationSupportDirectory + "/");
    _clientsResourcePath = QDir::toNativeSeparators(applicationSupportDirectory + "/" + _resourcePath);

    _assignmentClientExecutablePath = QDir::toNativeSeparators(_clientsLaunchPath + _assignmentClientExecutable);
    if (_platform == "win") {
        _assignmentClientExecutablePath.append(".exe");
    }
    _domainServerExecutablePath = QDir::toNativeSeparators(_clientsLaunchPath + _domainServerExecutable);
    if (_platform == "win") {
        _domainServerExecutablePath.append(".exe");
    }

    _defaultDomain = "localhost";
    _logsPath = QDir::toNativeSeparators(_clientsLaunchPath + "logs/");
    _availableAssignmentTypes.insert("audio-mixer", 0);
    _availableAssignmentTypes.insert("avatar-mixer", 1);
    _availableAssignmentTypes.insert("entity-server", 6);

    // allow user to override path to binaries so that they can run their own builds
    _hifiBuildDirectory = "";

    _domainServerBaseUrl = "http://localhost:40100";
}


void GlobalData::setHifiBuildDirectory(const QString hifiBuildDirectory) {
    _hifiBuildDirectory = hifiBuildDirectory;
    _clientsLaunchPath = QDir::toNativeSeparators(_hifiBuildDirectory + "/assignment-client/");
    _clientsResourcePath = QDir::toNativeSeparators(_clientsLaunchPath + "/" + _resourcePath);
    _logsPath = QDir::toNativeSeparators(_clientsLaunchPath + "logs/");
    _assignmentClientExecutablePath = QDir::toNativeSeparators(_clientsLaunchPath + _assignmentClientExecutable);
    _domainServerExecutablePath = QDir::toNativeSeparators(_hifiBuildDirectory + "/domain-server/" + _domainServerExecutable);
}
