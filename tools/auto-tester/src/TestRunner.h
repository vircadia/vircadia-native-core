//
//  Downloader.h
//
//  Created by Nissim Hadar on 1 Sep 2018.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_testRunner_h
#define hifi_testRunner_h

#include <QObject>
#include <QDir>

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

private:
    QDir _appDataFolder;
    QDir _savedAppDataFolder;

    QString _tempDirectory;

    Downloader* _downloader;

    const QString _uniqueFolderName{ "fgadhcUDHSFaidsfh3478JJJFSDFIUSOEIrf" };
    const QString _installerFilename{ "HighFidelity-Beta-latest-dev.exe" };

    BusyWindow _busyWindow;
};

#endif  // hifi_testRunner_h