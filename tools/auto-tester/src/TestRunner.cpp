//
//  TestRunner.cpp
//
//  Created by Nissim Hadar on 1 Sept 2018.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "TestRunner.h"

#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>

#include "ui/AutoTester.h"
extern AutoTester* autoTester;

TestRunner::TestRunner(QObject *parent) : QObject(parent) {
}

void TestRunner::run() {
    saveExistingHighFidelityAppDataFolder();
    selectTemporaryFolder();
    
    QStringList urls;
    urls << INSTALLER_URL;

    QStringList filenames;
    filenames << INSTALLER_FILENAME;

    autoTester->downloadFiles(urls, _tempFolder, filenames, (void *)this);

    // installerDownloadComplete will run after download complete
}

void TestRunner::installerDownloadComplete() {
    runInstaller();
    createSnapshotFolder();

    restoreHighFidelityAppDataFolder(); 
}

void TestRunner::runInstaller() {
    // Qt cannot start an installation process using QProcess::start (Qt Bug 9761)
    // To allow installation, the installer is run using the `system` command
    QStringList arguments{ QStringList() << QString("/S") << QString("/D=") + QDir::toNativeSeparators(_tempFolder) };
    
    QString installerFullPath = _tempFolder + "/" + INSTALLER_FILENAME;
    QString commandLine = QDir::toNativeSeparators(installerFullPath + " /S /D=" + _tempFolder);

    system(commandLine.toStdString().c_str());
}

void TestRunner::saveExistingHighFidelityAppDataFolder() {
    QString dataDirectory{ "NOT FOUND" };

#ifdef Q_OS_WIN
    dataDirectory = qgetenv("USERPROFILE") + "\\AppData\\Roaming";
#endif

    _appDataFolder = dataDirectory + "\\High Fidelity";

    if (!_appDataFolder.exists()) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "The High Fidelity data folder was not found in " + dataDirectory);
        exit(-1);
    }

    // The original folder is saved in a unique name
    _savedAppDataFolder = dataDirectory + "/" + UNIQUE_FOLDER_NAME;
    _appDataFolder.rename(_appDataFolder.path(), _savedAppDataFolder.path());

    QDir().mkdir(_appDataFolder.path());
}

void TestRunner::restoreHighFidelityAppDataFolder() {
    QDir().rmdir(_appDataFolder.path());

    _appDataFolder.rename(_savedAppDataFolder.path(), _appDataFolder.path());
}

void TestRunner::selectTemporaryFolder() {
    QString previousSelection = _tempFolder;
    QString parent = previousSelection.left(previousSelection.lastIndexOf('/'));
    if (!parent.isNull() && parent.right(1) != "/") {
        parent += "/";
    }

    _tempFolder =
        QFileDialog::getExistingDirectory(nullptr, "Please select a temporary folder for installation", parent, QFileDialog::ShowDirsOnly);

    // If user canceled then restore previous selection and return
    if (_tempFolder == "") {
        _tempFolder = previousSelection;
        return;
    }
}

void TestRunner::createSnapshotFolder() {
    QDir().mkdir(_tempFolder + "/" + SNAPSHOT_FOLDER_NAME);
}