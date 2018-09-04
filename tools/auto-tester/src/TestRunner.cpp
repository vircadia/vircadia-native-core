//
//  TestRunner.cpp
//
//  Created by Nissim Hadar on 1 Sep 2018.
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
    selectTemporaryFolder();
    runInstaller();




    saveExistingHighFidelityAppDataFolder();
    selectTemporaryFolder();
    
    QStringList urls;
    urls << "http://builds.highfidelity.com/HighFidelity-Beta-latest-dev.exe";

    QStringList filenames;
    filenames << _installerFilename;

    autoTester->downloadFiles(urls, _tempDirectory, filenames, (void *)this);

    // Will continue after download complete
}

void TestRunner::installerDownloadComplete() {
    runInstaller();

    restoreHighFidelityAppDataFolder(); 
}

void TestRunner::runInstaller() {
    // Qt cannot start an installation process using QProcess::start (Qt Bug 9761)
    // To allow installation, the installer is run using the `system` command
    QStringList arguments{ QStringList() << QString("/S") << QString("/D=") + QDir::toNativeSeparators(_tempDirectory) };
    
    QString installerFullPath = _tempDirectory + "/" + _installerFilename;
    QString commandLine = installerFullPath + " /S /D=" +  QDir::toNativeSeparators(_tempDirectory);
    system(commandLine.toStdString().c_str());
    int i = 34;
    //qint64 pid;
    //QProcess::startDetached(installerFullPath, arguments, QString(), &pid);
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
    _savedAppDataFolder = dataDirectory + "/" + _uniqueFolderName;
    _appDataFolder.rename(_appDataFolder.path(), _savedAppDataFolder.path());

    QDir().mkdir(_appDataFolder.path());
}

void TestRunner::restoreHighFidelityAppDataFolder() {
    QDir().rmdir(_appDataFolder.path());

    _appDataFolder.rename(_savedAppDataFolder.path(), _appDataFolder.path());
}

void TestRunner::selectTemporaryFolder() {
    QString previousSelection = _tempDirectory;
    QString parent = previousSelection.left(previousSelection.lastIndexOf('/'));
    if (!parent.isNull() && parent.right(1) != "/") {
        parent += "/";
    }

    _tempDirectory =
        QFileDialog::getExistingDirectory(nullptr, "Please select a temporary folder for installation", parent, QFileDialog::ShowDirsOnly);

    // If user canceled then restore previous selection and return
    if (_tempDirectory == "") {
        _tempDirectory = previousSelection;
        return;
    }
}