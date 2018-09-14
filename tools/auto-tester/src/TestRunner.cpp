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

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#endif

TestRunner::TestRunner(std::vector<QCheckBox*> dayCheckboxes,
                       std::vector<QCheckBox*> timeEditCheckboxes,
                       std::vector<QTimeEdit*> timeEdits,
                       QLabel* workingFolderLabel,
                       QObject* parent) :
    QObject(parent) 
{
    _dayCheckboxes = dayCheckboxes;
    _timeEditCheckboxes = timeEditCheckboxes;
    _timeEdits = timeEdits;
    _workingFolderLabel = workingFolderLabel;

    installerThread = new QThread();
    interfaceThread = new QThread();
}

void TestRunner::setWorkingFolder() {
    // Everything will be written to this folder
    QString previousSelection = _workingFolder;
    QString parent = previousSelection.left(previousSelection.lastIndexOf('/'));
    if (!parent.isNull() && parent.right(1) != "/") {
        parent += "/";
    }

    _workingFolder = QFileDialog::getExistingDirectory(nullptr, "Please select a temporary folder for installation", parent,
                                                    QFileDialog::ShowDirsOnly);

    // If user canceled then restore previous selection and return
    if (_workingFolder == "") {
        _workingFolder = previousSelection;
        return;
    }

    _installationFolder = _workingFolder + "/High Fidelity";
    _logFile.setFileName(_workingFolder + "/log.txt");

    autoTester->enableRunTabControls();
    _workingFolderLabel->setText(QDir::toNativeSeparators(_workingFolder));

    // The time is checked every 30 seconds for automatic test start
    _timer = new QTimer(this);
    connect(_timer, SIGNAL(timeout()), this, SLOT(checkTime()));
    _timer->start(30 * 1000);  //time specified in ms
}

void TestRunner::run() {
    _testStartDateTime = QDateTime::currentDateTime();
    _automatedTestIsRunning = true;

    // Initial setup
    _branch = autoTester->getSelectedBranch();
    _user = autoTester->getSelectedUser();

    // This will be restored at the end of the tests
    saveExistingHighFidelityAppDataFolder();

    // Download the latest High Fidelity installer and build XML.
    QStringList urls;
    urls << INSTALLER_URL << BUILD_XML_URL;

    QStringList filenames;
    filenames << INSTALLER_FILENAME << BUILD_XML_FILENAME;

    updateStatusLabel("Downloading installer");

    autoTester->downloadFiles(urls, _workingFolder, filenames, (void*)this);

    // `installerDownloadComplete` will run after download has completed
}

void TestRunner::installerDownloadComplete() {
    appendLog(QString("Tests started at ") + QString::number(_testStartDateTime.time().hour()) + ":" +
              QString("%1").arg(_testStartDateTime.time().minute(), 2, 10, QChar('0')) + ", on " +
              _testStartDateTime.date().toString("ddd, MMM d, yyyy"));

    updateStatusLabel("Installing");

    // Kill any existing processes that would interfere with installation
    killProcesses();

    runInstaller();
}

void TestRunner::runInstaller() {
    // Qt cannot start an installation process using QProcess::start (Qt Bug 9761)
    // To allow installation, the installer is run using the `system` command

    QStringList arguments{ QStringList() << QString("/S") << QString("/D=") + QDir::toNativeSeparators(_installationFolder) };

    QString installerFullPath = _workingFolder + "/" + INSTALLER_FILENAME;

    QString commandLine =
        QDir::toNativeSeparators(installerFullPath) + " /S /D=" + QDir::toNativeSeparators(_installationFolder);

    worker = new Worker(commandLine);

    worker->moveToThread(installerThread);
    connect(installerThread, SIGNAL(started()), worker, SLOT(process()));
    connect(worker, SIGNAL(finished()), this, SLOT(installationComplete()));
    installerThread->start();
}

void TestRunner::installationComplete() {
    disconnect(installerThread, SIGNAL(started()), worker, SLOT(process()));
    disconnect(worker, SIGNAL(finished()), this, SLOT(installationComplete()));
    delete worker;

    createSnapshotFolder();

    updateStatusLabel("Running tests");

    startLocalServerProcesses();
    runInterfaceWithTestScript();
}

void TestRunner::saveExistingHighFidelityAppDataFolder() {
    QString dataDirectory{ "NOT FOUND" };

#ifdef Q_OS_WIN
    dataDirectory = qgetenv("USERPROFILE") + "\\AppData\\Roaming";
#endif

    _appDataFolder = dataDirectory + "\\High Fidelity";

    if (_appDataFolder.exists()) {
        // The original folder is saved in a unique name
        _savedAppDataFolder = dataDirectory + "/" + UNIQUE_FOLDER_NAME;
        _appDataFolder.rename(_appDataFolder.path(), _savedAppDataFolder.path());
    }

    // Copy an "empty" AppData folder (i.e. no entities)
    copyFolder(QDir::currentPath() + "/AppDataHighFidelity", _appDataFolder.path());
}

void TestRunner::createSnapshotFolder() {
    _snapshotFolder = _workingFolder + "/" + SNAPSHOT_FOLDER_NAME;

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
#ifdef Q_OS_WIN
    try {
        QStringList processesToKill = QStringList() << "interface.exe"
                                                    << "assignment-client.exe"
                                                    << "domain-server.exe"
                                                    << "server-console.exe";

        // Loop until all pending processes to kill have actually died
        QStringList pendingProcessesToKill;
        do {
            pendingProcessesToKill.clear();

            // Get list of running tasks
            HANDLE processSnapHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (processSnapHandle == INVALID_HANDLE_VALUE) {
                throw("Process snapshot creation failure");
            }

            PROCESSENTRY32 processEntry32;
            processEntry32.dwSize = sizeof(PROCESSENTRY32);
            if (!Process32First(processSnapHandle, &processEntry32)) {
                CloseHandle(processSnapHandle);
                throw("Process32First failed");
            }

            // Kill any task in the list
            do {
                foreach (QString process, processesToKill)
                    if (QString(processEntry32.szExeFile) == process) {
                        QString commandLine = "taskkill /im " + process + " /f >nul";
                        system(commandLine.toStdString().c_str());
                        pendingProcessesToKill << process;
                    }
            } while (Process32Next(processSnapHandle, &processEntry32));

            QThread::sleep(2);
        } while (!pendingProcessesToKill.isEmpty());

    } catch (QString errorMessage) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), errorMessage);
        exit(-1);
    } catch (...) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "unknown error");
        exit(-1);
    }
#endif
}

void TestRunner::startLocalServerProcesses() {
#ifdef Q_OS_WIN
    QString commandLine;

    commandLine = "start \"domain-server.exe\" \"" + QDir::toNativeSeparators(_installationFolder) + "\\domain-server.exe\"";
    system(commandLine.toStdString().c_str());

    commandLine =
        "start \"assignment-client.exe\" \"" + QDir::toNativeSeparators(_installationFolder) + "\\assignment-client.exe\" -n 6";
    system(commandLine.toStdString().c_str());
#endif
    // Give server processes time to stabilize
    QThread::sleep(12);
}

void TestRunner::runInterfaceWithTestScript() {
    QString commandLine = QString("\"") + QDir::toNativeSeparators(_installationFolder) +
                          "\\interface.exe\" --url hifi://localhost --testScript https://raw.githubusercontent.com/" + _user +
                          "/hifi_tests/" + _branch + "/tests/testRecursive.js quitWhenFinished --testResultsLocation " +
                          _snapshotFolder;

    worker = new Worker(commandLine);

    worker->moveToThread(interfaceThread);
    connect(interfaceThread, SIGNAL(started()), worker, SLOT(process()));
    connect(worker, SIGNAL(finished()), this, SLOT(interfaceExecutionComplete()));
    interfaceThread->start();
}

void TestRunner::interfaceExecutionComplete() {
    disconnect(interfaceThread, SIGNAL(started()), worker, SLOT(process()));
    disconnect(worker, SIGNAL(finished()), this, SLOT(interfaceExecutionComplete()));
    delete worker;

    killProcesses();

    evaluateResults();

    // The High Fidelity AppData folder will be restored after evaluation has completed
}

void TestRunner::evaluateResults() {
    updateStatusLabel("Evaluating results");
    autoTester->startTestsEvaluation(false, true, _snapshotFolder, _branch, _user);
}

void TestRunner::automaticTestRunEvaluationComplete(QString zippedFolder) {
    addBuildNumberToResults(zippedFolder);
    restoreHighFidelityAppDataFolder();

    updateStatusLabel("Testing complete");

    QDateTime currentDateTime = QDateTime::currentDateTime();

    appendLog(QString("Tests completed at ") + QString::number(currentDateTime.time().hour()) + ":" +
              QString("%1").arg(currentDateTime.time().minute(), 2, 10, QChar('0')) + ", on " +
              currentDateTime.date().toString("ddd, MMM d, yyyy"));

    _automatedTestIsRunning = false;
}

void TestRunner::addBuildNumberToResults(QString zippedFolderName) {
    try {
        QDomDocument domDocument;
        QString filename{ _workingFolder + "/" + BUILD_XML_FILENAME };
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly) || !domDocument.setContent(&file)) {
            throw QString("Could not open " + filename);
        }

        QString platformOfInterest;
#ifdef Q_OS_WIN
        platformOfInterest = "windows";
#else if Q_OS_MAC
        platformOfInterest = "mac";
#endif
        QDomElement element = domDocument.documentElement();

        // Verify first element is "projects"
        if (element.tagName() != "projects") {
            throw("File seems to be in wrong format");
        }

        element = element.firstChild().toElement();
        if (element.tagName() != "project") {
            throw("File seems to be in wrong format");
        }

        if (element.attribute("name") != "interface") {
            throw("File is not from 'interface' build");
        }

        // Now loop over the platforms
        while (!element.isNull()) {
            element = element.firstChild().toElement();
            QString sdf = element.tagName();
            if (element.tagName() != "platform" || element.attribute("name") != platformOfInterest) {
                continue;
            }

            // Next element should be the build
            element = element.firstChild().toElement();
            if (element.tagName() != "build") {
                throw("File seems to be in wrong format");
            }

            // Next element should be the version
            element = element.firstChild().toElement();
            if (element.tagName() != "version") {
                throw("File seems to be in wrong format");
            }

            // Add the build number to the end of the filename
            QString build = element.text();
            QStringList filenameParts = zippedFolderName.split(".");
            QString augmentedFilename = filenameParts[0] + "(" + build + ")." + filenameParts[1];
            QFile::rename(zippedFolderName, augmentedFilename);
        }

    } catch (QString errorMessage) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), errorMessage);
        exit(-1);
    } catch (...) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "unknown error");
        exit(-1);
    }
}

void TestRunner::restoreHighFidelityAppDataFolder() {
    _appDataFolder.removeRecursively();

    if (_savedAppDataFolder != QDir()) {
        _appDataFolder.rename(_savedAppDataFolder.path(), _appDataFolder.path());
    }
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

void TestRunner::checkTime() {
    // No processing is done if a test is running
    if (_automatedTestIsRunning) {
        return;
    }

    QDateTime now = QDateTime::currentDateTime();

    // Check day of week
    if (!_dayCheckboxes.at(now.date().dayOfWeek() - 1)->isChecked()) {
        return;
    }

    // Check the time
    bool timeToRun{ false };
    QTime time = now.time();
    int h = time.hour();
    int m = time.minute();

    for (int i = 0; i < std::min(_timeEditCheckboxes.size(), _timeEdits.size()); ++i) {
        bool is = _timeEditCheckboxes[i]->isChecked();
        int hh = _timeEdits[i]->time().hour();
        int mm = _timeEdits[i]->time().minute();
        if (_timeEditCheckboxes[i]->isChecked() && (_timeEdits[i]->time().hour() == now.time().hour()) &&
            (_timeEdits[i]->time().minute() == now.time().minute())) {
            timeToRun = true;
            break;
        }
    }

    if (timeToRun) {
        run();
    }
}

void TestRunner::updateStatusLabel(const QString& message) {
    autoTester->updateStatusLabel(message);
}

void TestRunner::appendLog(const QString& message) {
    if (!_logFile.open(QIODevice::Append | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not open the log file");
        exit(-1);
    }

    _logFile.write(message.toStdString().c_str());
    _logFile.write("\n");
    _logFile.close();

    autoTester->appendLogWindow(message);
}

Worker::Worker(const QString commandLine) { 
    _commandLine = commandLine;
}

void Worker::process() {
    system(_commandLine.toStdString().c_str());
    emit finished();
}
