//
//  Downloader.h
//
//  Created by Nissim Hadar on 1 Sept 2018.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_testRunner_h
#define hifi_testRunner_h

#include <QObject>
#include <QDir>
#include <QProcess>

#include "Downloader.h"

class TestRunner : public QObject {
    Q_OBJECT
public:
    explicit TestRunner(QObject* parent = 0);

    void run();
    void installerDownloadComplete();
    void runInstaller();

    void saveExistingHighFidelityAppDataFolder();
    void restoreHighFidelityAppDataFolder();
    void selectTemporaryFolder();
    void createSnapshotFolder();
    void killProcesses();
    void startLocalServerProcesses();
    void runInterfaceWithTestScript();
    void evaluateResults();
    void automaticTestRunEvaluationComplete(QString zippedFolderName);
    void addBuildNumberToResults(QString zippedFolderName);

    void copyFolder(const QString& source, const QString& destination);

private:
    QDir _appDataFolder;
    QDir _savedAppDataFolder;

    QString _tempFolder;
    QString _snapshotFolder;

    QString _installationFolder;

    Downloader* _downloader;

    const QString UNIQUE_FOLDER_NAME{ "fgadhcUDHSFaidsfh3478JJJFSDFIUSOEIrf" };
    const QString SNAPSHOT_FOLDER_NAME{ "snapshots" };

    const QString INSTALLER_URL{ "http://builds.highfidelity.com/HighFidelity-Beta-latest-dev.exe" };
    const QString INSTALLER_FILENAME{ "HighFidelity-Beta-latest-dev.exe" };

    const QString BUILD_XML_URL{ "https://highfidelity.com/dev-builds.xml" };
    const QString BUILD_XML_FILENAME{ "dev-builds.xml" };

    QString _branch;
    QString _user;
};

#endif  // hifi_testRunner_h