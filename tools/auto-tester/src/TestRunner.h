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
#include "ui/BusyWindow.h"

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
    void killProcessByName(QString processName);
    void startLocalServerProcesses();
    void runInterfaceWithTestScript();

private:
    QDir _appDataFolder;
    QDir _savedAppDataFolder;

    QString _tempFolder;
    QString _snapshotFolder;

    Downloader* _downloader;

    const QString UNIQUE_FOLDER_NAME{ "fgadhcUDHSFaidsfh3478JJJFSDFIUSOEIrf" };
    const QString SNAPSHOT_FOLDER_NAME{ "snapshots" };

    const QString INSTALLER_URL{ "http://builds.highfidelity.com/HighFidelity-Beta-latest-dev.exe" };
    const QString INSTALLER_FILENAME{ "HighFidelity-Beta-latest-dev.exe" };

    BusyWindow _busyWindow;
};

#endif  // hifi_testRunner_h