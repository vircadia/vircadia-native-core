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

#include "ui/Nitpick.h"
extern Nitpick* nitpick;

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#endif

// TODO: for debug
#include <iostream>

TestRunner::TestRunner(std::vector<QCheckBox*> dayCheckboxes,
                       std::vector<QCheckBox*> timeEditCheckboxes,
                       std::vector<QTimeEdit*> timeEdits,
                       QLabel* workingFolderLabel,
                       QCheckBox* runServerless,
                       QCheckBox* runLatest,
                       QLineEdit* url,
                       QPushButton* runNow,
                       QObject* parent) :
    QObject(parent) {
    _dayCheckboxes = dayCheckboxes;
    _timeEditCheckboxes = timeEditCheckboxes;
    _timeEdits = timeEdits;
    _workingFolderLabel = workingFolderLabel;
    _runServerless = runServerless;
    _runLatest = runLatest;
    _url = url;
    _runNow = runNow;

    _installerThread = new QThread();
    _installerWorker = new Worker();
        
    _installerWorker->moveToThread(_installerThread);
    _installerThread->start();
    connect(this, SIGNAL(startInstaller()), _installerWorker, SLOT(runCommand()));
    connect(_installerWorker, SIGNAL(commandComplete()), this, SLOT(installationComplete()));

    _interfaceThread = new QThread();
    _interfaceWorker = new Worker();

    _interfaceThread->start();
    _interfaceWorker->moveToThread(_interfaceThread);
    connect(this, SIGNAL(startInterface()), _interfaceWorker, SLOT(runCommand()));
    connect(_interfaceWorker, SIGNAL(commandComplete()), this, SLOT(interfaceExecutionComplete()));
}

TestRunner::~TestRunner() {
    delete _installerThread;
    delete _installerWorker;

    delete _interfaceThread;
    delete _interfaceWorker;

    if (_timer) {
        delete _timer;
    }
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

#ifdef Q_OS_WIN
    _installationFolder = _workingFolder + "/High Fidelity";
#elif defined Q_OS_MAC
    _installationFolder = _workingFolder + "/High_Fidelity";
#endif

    _logFile.setFileName(_workingFolder + "/log.txt");

    nitpick->enableRunTabControls();
    _workingFolderLabel->setText(QDir::toNativeSeparators(_workingFolder));

    _timer = new QTimer(this);
    connect(_timer, SIGNAL(timeout()), this, SLOT(checkTime()));
    _timer->start(30 * 1000);  //time specified in ms
    
#ifdef Q_OS_MAC
    // Create MAC shell scripts
    QFile script;
    
    // This script waits for a process to start
    script.setFileName(_workingFolder + "/waitForStart.sh");
    if (!script.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not open 'waitForStart.sh'");
        exit(-1);
    }
    
    script.write("#!/bin/sh\n\n");
    script.write("PROCESS=\"$1\"\n");
    script.write("until (pgrep -x $PROCESS >nul)\n");
    script.write("do\n");
    script.write("\techo waiting for \"$1\" to start\n");
    script.write("\tsleep 2\n");
    script.write("done\n");
    script.write("echo \"$1\" \"started\"\n");
    script.close();
    script.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);

    // The Mac shell command returns immediately.  This little script waits for a process to finish
    script.setFileName(_workingFolder + "/waitForFinish.sh");
    if (!script.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not open 'waitForFinish.sh'");
        exit(-1);
    }
    
    script.write("#!/bin/sh\n\n");
    script.write("PROCESS=\"$1\"\n");
    script.write("while (pgrep -x $PROCESS >nul)\n");
    script.write("do\n");
    script.write("\techo waiting for \"$1\" to finish\n");
    script.write("\tsleep 2\n");
    script.write("done\n");
    script.write("echo \"$1\" \"finished\"\n");
    script.close();
    script.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    
    // Create an AppleScript to resize Interface.  This is needed so that snapshots taken
    // with the primary camera will be the correct size.
    // This will be run from a normal shell script
    script.setFileName(_workingFolder + "/setInterfaceSizeAndPosition.scpt");
    if (!script.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not open 'setInterfaceSizeAndPosition.scpt'");
        exit(-1);
    }
    
    script.write("set width to 960\n");
    script.write("set height to 540\n");
    script.write("set x to 100\n");
    script.write("set y to 100\n\n");
    script.write("tell application \"System Events\" to tell application process \"interface\" to tell window 1 to set {size, position} to {{width, height}, {x, y}}\n");

    script.close();
    script.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);

    script.setFileName(_workingFolder + "/setInterfaceSizeAndPosition.sh");
    if (!script.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not open 'setInterfaceSizeAndPosition.sh'");
        exit(-1);
    }
    
    script.write("#!/bin/sh\n\n");
    script.write("echo resizing interface\n");
    script.write(("osascript " + _workingFolder + "/setInterfaceSizeAndPosition.scpt\n").toStdString().c_str());
    script.write("echo resize complete\n");
    script.close();
    script.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
#endif
}

void TestRunner::run() {
    _runNow->setEnabled(false);

    _testStartDateTime = QDateTime::currentDateTime();
    _automatedTestIsRunning = true;

    // Initial setup
    _branch = nitpick->getSelectedBranch();
    _user = nitpick->getSelectedUser();

    // This will be restored at the end of the tests
    saveExistingHighFidelityAppDataFolder();

    // Download the latest High Fidelity build XML.
    //      Note that this is not needed for PR builds (or whenever `Run Latest` is unchecked)
    //      It is still downloaded, to simplify the flow
    QStringList urls;
    QStringList filenames;

    urls << DEV_BUILD_XML_URL;
    filenames << DEV_BUILD_XML_FILENAME;

    updateStatusLabel("Downloading Build XML");

    buildXMLDownloaded = false;
    nitpick->downloadFiles(urls, _workingFolder, filenames, (void*)this);

    // `downloadComplete` will run after download has completed
}

void TestRunner::downloadComplete() {
    if (!buildXMLDownloaded) {
        // Download of Build XML has completed
        buildXMLDownloaded = true;

        // Download the High Fidelity installer
        QStringList urls;
        QStringList filenames;
        if (_runLatest->isChecked()) {
            parseBuildInformation();

            _installerFilename = INSTALLER_FILENAME_LATEST;

            urls << _buildInformation.url;
            filenames << _installerFilename;
        } else {
            QString urlText = _url->text();
            urls << urlText;
            _installerFilename = getInstallerNameFromURL(urlText);
            filenames << _installerFilename;
        }

        updateStatusLabel("Downloading installer");

        nitpick->downloadFiles(urls, _workingFolder, filenames, (void*)this);

        // `downloadComplete` will run again after download has completed

    } else {
        // Download of Installer has completed
        appendLog(QString("Tests started at ") + QString::number(_testStartDateTime.time().hour()) + ":" +
                  QString("%1").arg(_testStartDateTime.time().minute(), 2, 10, QChar('0')) + ", on " +
                  _testStartDateTime.date().toString("ddd, MMM d, yyyy"));

        updateStatusLabel("Installing");

        // Kill any existing processes that would interfere with installation
        killProcesses();

        runInstaller();
    }
}

void TestRunner::runInstaller() {
    // Qt cannot start an installation process using QProcess::start (Qt Bug 9761)
    // To allow installation, the installer is run using the `system` command

    QStringList arguments{ QStringList() << QString("/S") << QString("/D=") + QDir::toNativeSeparators(_installationFolder) };

    QString installerFullPath = _workingFolder + "/" + _installerFilename;

    QString commandLine;
#ifdef Q_OS_WIN
    commandLine = "\"" + QDir::toNativeSeparators(installerFullPath) + "\"" + " /S /D=" + QDir::toNativeSeparators(_installationFolder);
#elif defined Q_OS_MAC
    // Create installation shell script
    QFile script;
    script.setFileName(_workingFolder + "/install_app.sh");
    if (!script.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not open 'install_app.sh'");
        exit(-1);
    }
    
    if (!QDir().exists(_installationFolder)) {
        QDir().mkdir(_installationFolder);
    }
    
    // This script installs High Fidelity.  It is run as "yes | install_app.sh... so "yes" is killed at the end
    script.write("#!/bin/sh\n\n");
    script.write("VOLUME=`hdiutil attach \"$1\" | grep Volumes | awk '{print $3}'`\n");
    
    QString folderName {"High Fidelity"};
    if (!_runLatest->isChecked()) {
        folderName += QString(" - ") + getPRNumberFromURL(_url->text());
    }

    script.write((QString("cp -rf \"$VOLUME/") + folderName + "/interface.app\" \"" + _workingFolder + "/High_Fidelity/\"\n").toStdString().c_str());
    script.write((QString("cp -rf \"$VOLUME/") + folderName + "/Sandbox.app\" \""   + _workingFolder + "/High_Fidelity/\"\n").toStdString().c_str());
    
    script.write("hdiutil detach \"$VOLUME\"\n");
    script.write("killall yes\n");
    script.close();
    script.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    commandLine = "yes | " + _workingFolder + "/install_app.sh " + installerFullPath;
#endif
    appendLog(commandLine);
    _installerWorker->setCommandLine(commandLine);
    emit startInstaller();
}

void TestRunner::installationComplete() {
    verifyInstallationSucceeded();

    createSnapshotFolder();

    updateStatusLabel("Running tests");

    if (!_runServerless->isChecked()) {
        startLocalServerProcesses();
    }

    runInterfaceWithTestScript();
}

void TestRunner::verifyInstallationSucceeded() {
    // Exit if the executables are missing.
    // On Windows, the reason is probably that UAC has blocked the installation.  This is treated as a critical error
#ifdef Q_OS_WIN
    QFileInfo interfaceExe(QDir::toNativeSeparators(_installationFolder) + "\\interface.exe");
    QFileInfo assignmentClientExe(QDir::toNativeSeparators(_installationFolder) + "\\assignment-client.exe");
    QFileInfo domainServerExe(QDir::toNativeSeparators(_installationFolder) + "\\domain-server.exe");

    if (!interfaceExe.exists() || !assignmentClientExe.exists() || !domainServerExe.exists()) {
        QMessageBox::critical(0, "Installation of High Fidelity has failed", "Please verify that UAC has been disabled");
        exit(-1);
    }
#endif
}

void TestRunner::saveExistingHighFidelityAppDataFolder() {
#ifdef Q_OS_WIN
    QString dataDirectory{ "NOT FOUND" };

    dataDirectory = qgetenv("USERPROFILE") + "\\AppData\\Roaming";

    if (_runLatest->isChecked()) {
        _appDataFolder = dataDirectory + "\\High Fidelity";
    } else {
        // We are running a PR build
        _appDataFolder = dataDirectory + "\\High Fidelity - " + getPRNumberFromURL(_url->text());
    }

    _savedAppDataFolder = dataDirectory + "/" + UNIQUE_FOLDER_NAME;
    if (_savedAppDataFolder.exists()) {
        _savedAppDataFolder.removeRecursively();
    }

    if (_appDataFolder.exists()) {
        // The original folder is saved in a unique name
        _appDataFolder.rename(_appDataFolder.path(), _savedAppDataFolder.path());
    }

    // Copy an "empty" AppData folder (i.e. no entities)
    copyFolder(QDir::currentPath() + "/AppDataHighFidelity", _appDataFolder.path());
#elif defined Q_OS_MAC
    // TODO:  find Mac equivalent of AppData
#endif
}

void TestRunner::createSnapshotFolder() {
    _snapshotFolder = _workingFolder + "/" + SNAPSHOT_FOLDER_NAME;

    // Just delete all PNGs from the folder if it already exists
    if (QDir(_snapshotFolder).exists()) {
        // Note that we cannot use just a `png` filter, as the filenames include periods
        // Also, delete any `jpg` and `txt` files
        // The idea is to leave only previous zipped result folders
        QDirIterator it(_snapshotFolder.toStdString().c_str());
        while (it.hasNext()) {
            QString filename = it.next();
            if (filename.right(4) == ".png" || filename.right(4) == ".jpg" || filename.right(4) == ".txt") {
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
#elif defined Q_OS_MAC
    QString commandLine;
    
    commandLine = QString("killall interface") + "; " + _workingFolder +"/waitForFinish.sh interface";
    system(commandLine.toStdString().c_str());
    
    commandLine = QString("killall Sandbox") + "; " + _workingFolder +"/waitForFinish.sh Sandbox";
    system(commandLine.toStdString().c_str());
    
    commandLine = QString("killall Console") + "; " + _workingFolder +"/waitForFinish.sh Console";
    system(commandLine.toStdString().c_str());
#endif
}

void TestRunner::startLocalServerProcesses() {
    QString commandLine;
    
#ifdef Q_OS_WIN
    commandLine =
        "start \"domain-server.exe\" \"" + QDir::toNativeSeparators(_installationFolder) + "\\domain-server.exe\"";
    system(commandLine.toStdString().c_str());

    commandLine =
        "start \"assignment-client.exe\" \"" + QDir::toNativeSeparators(_installationFolder) + "\\assignment-client.exe\" -n 6";
    system(commandLine.toStdString().c_str());

#elif defined Q_OS_MAC
    commandLine = "open \"" +_installationFolder + "/Sandbox.app\"";
    system(commandLine.toStdString().c_str());
#endif

    // Give server processes time to stabilize
    QThread::sleep(20);
}

void TestRunner::runInterfaceWithTestScript() {
    QString url = QString("hifi://localhost");
    if (_runServerless->isChecked()) {
        // Move to an empty area
        url = "file:///~serverless/tutorial.json";
    } else {
#ifdef Q_OS_WIN
        url = "hifi://localhost";
#elif defined Q_OS_MAC
        // TODO: Find out Mac equivalent of AppData, then this won't be needed
        url = "hifi://localhost/9999,9999,9999";
#endif
    }

    QString testScript =
        QString("https://raw.githubusercontent.com/") + _user + "/hifi_tests/" + _branch + "/tests/testRecursive.js";

    QString commandLine;
#ifdef Q_OS_WIN
    QString exeFile = QString("\"") + QDir::toNativeSeparators(_installationFolder) + "\\interface.exe\"";
    commandLine = exeFile +
    " --url " + url +
    " --no-updater" +
    " --no-login-suggestion"
    " --testScript " + testScript + " quitWhenFinished" +
    " --testResultsLocation " + _snapshotFolder;

    _interfaceWorker->setCommandLine(commandLine);
    emit startInterface();
#elif defined Q_OS_MAC
    // On The Mac, we need to resize Interface.  The Interface window opens a few seconds after the process
    // has started.
    // Before starting interface, start a process that will resize interface 10s after it opens
    // This is performed by creating a bash script that runs to processes
    QFile script;
    script.setFileName(_workingFolder + "/runInterfaceTests.sh");
    if (!script.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not open 'runInterfaceTests.sh'");
        exit(-1);
    }
    
    script.write("#!/bin/sh\n\n");

    commandLine = _workingFolder +"/waitForStart.sh interface && sleep 10 && " + _workingFolder +"/setInterfaceSizeAndPosition.sh &\n";
    script.write(commandLine.toStdString().c_str());

    commandLine =
        "open \"" +_installationFolder + "/interface.app\" --args" +
        " --url " + url +
        " --no-updater" +
        " --no-login-suggestion"
        " --testScript " + testScript + " quitWhenFinished" +
        " --testResultsLocation " + _snapshotFolder +
        " && " + _workingFolder +"/waitForFinish.sh interface";

    script.write(commandLine.toStdString().c_str());

    script.close();
    script.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);

    commandLine = _workingFolder + "/runInterfaceTests.sh";
    _interfaceWorker->setCommandLine(commandLine);

    emit startInterface();
#endif
    
    // Helpful for debugging
    appendLog(commandLine);
}

void TestRunner::interfaceExecutionComplete() {
    killProcesses();

    QFileInfo testCompleted(QDir::toNativeSeparators(_snapshotFolder) +"/tests_completed.txt");
    if (!testCompleted.exists()) {
        QMessageBox::critical(0, "Tests not completed", "Interface seems to have crashed before completion of the test scripts\nExisting images will be evaluated");
    }

    evaluateResults();

    // The High Fidelity AppData folder will be restored after evaluation has completed
}

void TestRunner::evaluateResults() {
    updateStatusLabel("Evaluating results");
    nitpick->startTestsEvaluation(false, true, _snapshotFolder, _branch, _user);
}

void TestRunner::automaticTestRunEvaluationComplete(QString zippedFolder, int numberOfFailures) {
    addBuildNumberToResults(zippedFolder);
    restoreHighFidelityAppDataFolder();

    updateStatusLabel("Testing complete");

    QDateTime currentDateTime = QDateTime::currentDateTime();

    QString completionText = QString("Tests completed at ") + QString::number(currentDateTime.time().hour()) + ":" +
                             QString("%1").arg(currentDateTime.time().minute(), 2, 10, QChar('0')) + ", on " +
                             currentDateTime.date().toString("ddd, MMM d, yyyy");

    if (numberOfFailures == 0) {
        completionText += "; no failures";
    } else if (numberOfFailures == 1) {
        completionText += "; 1 failure";
    } else {
        completionText += QString("; ") + QString::number(numberOfFailures) + " failures";
    }
    appendLog(completionText);

    _automatedTestIsRunning = false;

    _runNow->setEnabled(true);
}

void TestRunner::addBuildNumberToResults(QString zippedFolderName) {
    QString augmentedFilename;
    if (!_runLatest->isChecked()) {
        augmentedFilename = zippedFolderName.replace("local", getPRNumberFromURL(_url->text()));
    } else {
        augmentedFilename = zippedFolderName.replace("local", _buildInformation.build);
    }
    QFile::rename(zippedFolderName, augmentedFilename);
}

void TestRunner::restoreHighFidelityAppDataFolder() {
#ifdef Q_OS_WIN
    _appDataFolder.removeRecursively();

    if (_savedAppDataFolder != QDir()) {
        _appDataFolder.rename(_savedAppDataFolder.path(), _appDataFolder.path());
    }
#elif defined Q_OS_MAC
    // TODO:  find Mac equivalent of AppData
#endif
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

    for (size_t i = 0; i < std::min(_timeEditCheckboxes.size(), _timeEdits.size()); ++i) {
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
    nitpick->updateStatusLabel(message);
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

    nitpick->appendLogWindow(message);
}

QString TestRunner::getInstallerNameFromURL(const QString& url) {
    // An example URL: https://deployment.highfidelity.com/jobs/pr-build/label%3Dwindows/13023/HighFidelity-Beta-Interface-PR14006-be76c43.exe
    // On Mac, replace `exe` with `dmg`
    try {
        QStringList urlParts = url.split("/");
        return urlParts[urlParts.size() - 1];
    } catch (QString errorMessage) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), errorMessage);
        exit(-1);
    } catch (...) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "unknown error");
        exit(-1);
    }
}

QString TestRunner::getPRNumberFromURL(const QString& url) {
    try {
        QStringList urlParts = url.split("/");
        QStringList filenameParts = urlParts[urlParts.size() - 1].split("-");
        if (filenameParts.size() <= 3) {
#ifdef Q_OS_WIN
            throw "URL not in expected format, should look like `https://deployment.highfidelity.com/jobs/pr-build/label%3Dwindows/13023/HighFidelity-Beta-Interface-PR14006-be76c43.exe`";
#elif defined Q_OS_MAC
            throw "URL not in expected format, should look like `https://deployment.highfidelity.com/jobs/pr-build/label%3Dwindows/13023/HighFidelity-Beta-Interface-PR14006-be76c43.dmg`";
#endif
        }
        return filenameParts[filenameParts.size() - 2];
    } catch (QString errorMessage) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), errorMessage);
        exit(-1);
    } catch (...) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "unknown error");
        exit(-1);
    }
}

void TestRunner::parseBuildInformation() {
    try {
        QDomDocument domDocument;
        QString filename{ _workingFolder + "/" + DEV_BUILD_XML_FILENAME };
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly) || !domDocument.setContent(&file)) {
            throw QString("Could not open " + filename);
        }

        QString platformOfInterest;
#ifdef Q_OS_WIN
        platformOfInterest = "windows";
#elif defined(Q_OS_MAC)
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

        // Now loop over the platforms, looking for ours
        bool platformFound{ false };
        element = element.firstChild().toElement();
        while (!element.isNull()) {
            if (element.attribute("name") == platformOfInterest) {
                platformFound = true;
                break;
            }
            element = element.nextSibling().toElement();
        }

        if (!platformFound) {
            throw("File seems to be in wrong format - platform " + platformOfInterest + " not found");
        }

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
        _buildInformation.build = element.text();

        // First sibling should be stable_version
        element = element.nextSibling().toElement();
        if (element.tagName() != "stable_version") {
            throw("File seems to be in wrong format");
        }

        // Next sibling should be url
        element = element.nextSibling().toElement();
        if (element.tagName() != "url") {
            throw("File seems to be in wrong format");
        }
        _buildInformation.url = element.text();

    } catch (QString errorMessage) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), errorMessage);
        exit(-1);
    } catch (...) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "unknown error");
        exit(-1);
    }
}

void Worker::setCommandLine(const QString& commandLine) {
    _commandLine = commandLine;
}

int Worker::runCommand() {
    int result = system(_commandLine.toStdString().c_str());
    emit commandComplete();
    return result;
}
