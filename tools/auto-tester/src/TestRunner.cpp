//
//  Downloader.cpp
//
//  Created by Nissim Hadar on 1 Sep 2018.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "TestRunner.h"

#include <QtWidgets/QMessageBox>

TestRunner::TestRunner(QObject *parent) : QObject(parent) {
}

void TestRunner::run() {
    saveExistingHighFidelityAppDataFolder();

    ////////////////////////////////////////////////////////////////////////////////

    restoreHighFidelityAppDataFolder();
}

void TestRunner::saveExistingHighFidelityAppDataFolder() {
    QString dataDirectory{ "NOT FOUND" };

#ifdef Q_OS_WIN
    dataDirectory = qgetenv("USERPROFILE") + "\\AppData\\Roaming";
#endif

    appDataFolder = dataDirectory + "\\High Fidelity";

    if (!appDataFolder.exists()) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "The High Fidelity data folder was not found in " + dataDirectory);
        exit(-1);
    }

    // The original folder is saved in a unique name
    savedAppDataFolder = dataDirectory + "/fgadhcUDHSFaidsfh3478JJJFSDFIUSOEIrf";
    appDataFolder.rename(QDir::fromNativeSeparators(appDataFolder.path()), QDir::toNativeSeparators(savedAppDataFolder.path()));

    QDir().mkdir(appDataFolder.path());
}

void TestRunner::restoreHighFidelityAppDataFolder() {
    QDir().rmdir(appDataFolder.path());

    appDataFolder.rename(QDir::fromNativeSeparators(savedAppDataFolder.path()), QDir::toNativeSeparators(appDataFolder.path()));
}