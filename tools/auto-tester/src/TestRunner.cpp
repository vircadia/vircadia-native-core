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

#include <QThread>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>

#include "ui/AutoTester.h"
extern AutoTester* autoTester;

TestRunner::TestRunner(QObject *parent) : QObject(parent) {
}

void TestRunner::run() {
    // Initial setup
    _branch = autoTester->getSelectedBranch();
    _user = autoTester->getSelectedUser();

    // Everything will be written to this folder
    selectTemporaryFolder();

    // This will be restored at the end of the tests
    saveExistingHighFidelityAppDataFolder();
    
    // Download the latest High Fidelity installer
    QStringList urls;
    urls << INSTALLER_URL;

    QStringList filenames;
    filenames << INSTALLER_FILENAME;

    autoTester->downloadFiles(urls, _tempFolder, filenames, (void *)this);

    // `installerDownloadComplete` will run after download has completed
}

void TestRunner::installerDownloadComplete() {
    // Kill any existing processes that would interfere with installation
    killProcesses();

    runInstaller();
    
    createSnapshotFolder();

    startLocalServerProcesses();
    runInterfaceWithTestScript();
    killProcesses();

    evaluateResults();

    // The High Fidelity AppData folder will be restored after evaluation has completed
}

void TestRunner::runInstaller() {
    // Qt cannot start an installation process using QProcess::start (Qt Bug 9761)
    // To allow installation, the installer is run using the `system` command
    QStringList arguments{ QStringList() << QString("/S") << QString("/D=") + QDir::toNativeSeparators(_tempFolder) };
    
    QString installerFullPath = _tempFolder + "/" + INSTALLER_FILENAME;
    
    QString commandLine = QDir::toNativeSeparators(installerFullPath) + " /S /D=" + QDir::toNativeSeparators(_tempFolder);
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

    // Copy an "empty" AppData folder (i.e. no entities)
    copyFolder(QDir::currentPath() +  "/AppDataHighFidelity", _appDataFolder.path());
}

void TestRunner::restoreHighFidelityAppDataFolder() {
    _appDataFolder.removeRecursively();
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
    _snapshotFolder = _tempFolder + "/" + SNAPSHOT_FOLDER_NAME;

    // Just delete all PNGs from the folder if it already exists
    if (QDir(_snapshotFolder).exists()) {
        // Note that we cannot use just a `png` filter, as the filenames include periods
        QDirIterator it(_snapshotFolder.toStdString().c_str());
        while (it.hasNext()) {
            QString filename = it.next();
            if (filename.right(4) == ".png") {
                QFile::remove(filename);
            }
        }

    } else {
        QDir().mkdir(_snapshotFolder);
    }
}

void TestRunner::killProcesses() {
    killProcessByName("assignment-client.exe");
    killProcessByName("domain-server.exe");
    killProcessByName("server-console.exe");
}

void TestRunner::killProcessByName(QString processName) {
#ifdef Q_OS_WIN
    QString commandLine = "taskkill /im " + processName + " /f >nul";
    system(commandLine.toStdString().c_str());
#endif
}

void TestRunner::startLocalServerProcesses() {
#ifdef Q_OS_WIN
    QString commandLine;

    commandLine = "start \"domain-server.exe\" " + QDir::toNativeSeparators(_tempFolder) + "\\domain-server.exe";
    system(commandLine.toStdString().c_str());

    commandLine = "start \"assignment-client.exe\" " + QDir::toNativeSeparators(_tempFolder) + "\\assignment-client.exe -n 6";
    system(commandLine.toStdString().c_str());
#endif
    // Give server processes time to stabilize
    QThread::sleep(8);
}

void TestRunner::runInterfaceWithTestScript() {
#ifdef Q_OS_WIN
        QString commandLine = QDir::toNativeSeparators(_tempFolder) +
                              "\\interface.exe --url hifi://localhost --testScript https://raw.githubusercontent.com/" + _user +
                              "/hifi_tests/" + _branch + "/tests/testRecursive.js quitWhenFinished --testResultsLocation " +
                              _snapshotFolder;

    system(commandLine.toStdString().c_str());
#endif
}

void TestRunner::evaluateResults() {
    autoTester->startTestsEvaluation(false, true, _snapshotFolder, _branch, _user);
}

// Copies a folder recursively
void TestRunner::copyFolder(const QString& source, const QString& destination) {
    try {
        if (!QFileInfo(source).isDir()) {
            // just a file copy
            QFile::copy(source, destination);
        } else {
            QDir destinationDir(destination);
            if (!destinationDir.cdUp()) {
                throw("'source '" + source + "'seems to be a root folder");
            }

            if (!destinationDir.mkdir(QFileInfo(destination).fileName())) {
                throw("Could not create destination folder '" + destination + "'");
            }

            QStringList fileNames =
                QDir(source).entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);

            foreach (const QString& fileName, fileNames) {
                copyFolder(QString(source + "/" + fileName), QString(destination + "/" + fileName));
            }
        }
    } catch (QString errorMessage) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), errorMessage);
        exit(-1);
    } catch (...) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "unknown error");
        exit(-1);
    }
}

void TestRunner::automaticTestRunEvaluationComplete() {
    restoreHighFidelityAppDataFolder();
}
