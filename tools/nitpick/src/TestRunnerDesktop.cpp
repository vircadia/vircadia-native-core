//
//  TestRunnerDesktop.cpp
//
//  Created by Nissim Hadar on 1 Sept 2018.
//  Copyright 2013 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "TestRunnerDesktop.h"

#include <QThread>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#endif

#include "Nitpick.h"
extern Nitpick* nitpick;

TestRunnerDesktop::TestRunnerDesktop(
    std::vector<QCheckBox*> dayCheckboxes,
    std::vector<QCheckBox*> timeEditCheckboxes,
    std::vector<QTimeEdit*> timeEdits,
    QLabel* workingFolderLabel,
    QCheckBox* runServerless,
    QCheckBox* usePreviousInstallationOnMobileCheckBox,
    QCheckBox* runLatest,
    QLineEdit* url,
    QCheckBox* runFullSuite,
    QLineEdit* scriptURL,
    QPushButton* runNow,
    QLabel* statusLabel,

    QObject* parent
) : QObject(parent), TestRunner(workingFolderLabel, statusLabel, usePreviousInstallationOnMobileCheckBox, runLatest, url, runFullSuite, scriptURL)
{
    _dayCheckboxes = dayCheckboxes;
    _timeEditCheckboxes = timeEditCheckboxes;
    _timeEdits = timeEdits;
    _runServerless = runServerless;
    _runNow = runNow;

    _installerThread = new QThread();
    _installerWorker = new InstallerWorker();

    _installerWorker->moveToThread(_installerThread);
    _installerThread->start();
    connect(this, SIGNAL(startInstaller()), _installerWorker, SLOT(runCommand()));
    connect(_installerWorker, SIGNAL(commandComplete()), this, SLOT(installationComplete()));

    _interfaceThread = new QThread();
    _interfaceWorker = new InterfaceWorker();

    _interfaceThread->start();
    _interfaceWorker->moveToThread(_interfaceThread);
    connect(this, SIGNAL(startInterface()), _interfaceWorker, SLOT(runCommand()));
    connect(_interfaceWorker, SIGNAL(commandComplete()), this, SLOT(interfaceExecutionComplete()));
}

TestRunnerDesktop::~TestRunnerDesktop() {
    delete _installerThread;
    delete _installerWorker;

    delete _interfaceThread;
    delete _interfaceWorker;

    if (_timer) {
        delete _timer;
    }
}

void TestRunnerDesktop::setWorkingFolderAndEnableControls() {
    setWorkingFolder(_workingFolderLabel);

#ifdef Q_OS_WIN
    _installationFolder = _workingFolder + "/Vircadia";
#elif defined Q_OS_MAC
    _installationFolder = _workingFolder + "/Vircadia";
#endif

    nitpick->enableRunTabControls();

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

void TestRunnerDesktop::run() {
    _runNow->setEnabled(false);

    _testStartDateTime = QDateTime::currentDateTime();
    _automatedTestIsRunning = true;

    // Initial setup
    _branch = nitpick->getSelectedBranch();
    _user = nitpick->getSelectedUser();

    // This will be restored at the end of the tests
    saveExistingHighFidelityAppDataFolder();

    if (_usePreviousInstallationCheckBox->isChecked()) {
        installationComplete();
    } else {
        _statusLabel->setText("Downloading Build XML");
        downloadBuildXml((void*)this);

        downloadComplete();
    }
}

void TestRunnerDesktop::downloadComplete() {
    if (!buildXMLDownloaded) {
        // Download of Build XML has completed
        buildXMLDownloaded = true;

        // Download the Vircadia installer
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

        _statusLabel->setText("Downloading installer");

        _downloader->downloadFiles(urls, _workingFolder, filenames, (void*)this);

        downloadComplete();

    } else {
        // Download of Installer has completed
        appendLog(QString("Tests started at ") + QString::number(_testStartDateTime.time().hour()) + ":" +
                  QString("%1").arg(_testStartDateTime.time().minute(), 2, 10, QChar('0')) + ", on " +
                  _testStartDateTime.date().toString("ddd, MMM d, yyyy"));

        _statusLabel->setText("Installing");

        // Kill any existing processes that would interfere with installation
        killProcesses();

        runInstaller();
    }
}

void TestRunnerDesktop::runInstaller() {
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

    // This script installs Vircadia.  It is run as "yes | install_app.sh... so "yes" is killed at the end
    script.write("#!/bin/sh\n\n");
    script.write("VOLUME=`hdiutil attach \"$1\" | grep Volumes | awk '{print $3}'`\n");

    QString folderName {"Vircadia"};
    if (!_runLatest->isChecked()) {
        folderName += QString(" - ") + getPRNumberFromURL(_url->text());
    }

    script.write((QString("cp -Rf \"$VOLUME/") + folderName + "/interface.app\" \"" + _workingFolder + "/High_Fidelity/\"\n").toStdString().c_str());
    script.write((QString("cp -Rf \"$VOLUME/") + folderName + "/Sandbox.app\" \""   + _workingFolder + "/High_Fidelity/\"\n").toStdString().c_str());

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

void TestRunnerDesktop::installationComplete() {
    verifyInstallationSucceeded();

    createSnapshotFolder();

    _statusLabel->setText("Running tests");

    if (!_runServerless->isChecked()) {
        startLocalServerProcesses();
    }

    runInterfaceWithTestScript();
}

void TestRunnerDesktop::verifyInstallationSucceeded() {
    // Exit if the executables are missing.
#ifdef Q_OS_WIN
    QFileInfo interfaceExe(QDir::toNativeSeparators(_installationFolder) + "\\interface.exe");
    QFileInfo assignmentClientExe(QDir::toNativeSeparators(_installationFolder) + "\\assignment-client.exe");
    QFileInfo domainServerExe(QDir::toNativeSeparators(_installationFolder) + "\\domain-server.exe");

    if (!interfaceExe.exists() || !assignmentClientExe.exists() || !domainServerExe.exists()) {
        if (_runLatest->isChecked()) {
            // On Windows, the reason is probably that UAC has blocked the installation.  This is treated as a critical error
            QMessageBox::critical(0, "Installation of Vircadia has failed", "Please verify that UAC has been disabled");
            exit(-1);
        } else {
            QMessageBox::critical(0, "Installation of vircadia not found", "Please verify that working folder contains a proper installation");
        }
    }
#endif
}

void TestRunnerDesktop::saveExistingHighFidelityAppDataFolder() {
    QString dataDirectory{ "NOT FOUND" };
#ifdef Q_OS_WIN
    dataDirectory = qgetenv("USERPROFILE") + "\\AppData\\Roaming";
#elif defined Q_OS_MAC
    dataDirectory = QDir::homePath() + "/Library/Application Support";
#endif
    if (_runLatest->isChecked()) {
        _appDataFolder = dataDirectory + "/Vircadia";
    } else {
        // We are running a PR build
        _appDataFolder = dataDirectory + "/Vircadia - " + getPRNumberFromURL(_url->text());
    }

    _savedAppDataFolder = dataDirectory + "/" + UNIQUE_FOLDER_NAME;
    if (QDir(_savedAppDataFolder).exists()) {
        _savedAppDataFolder.removeRecursively();
    }
    if (_appDataFolder.exists()) {
        // The original folder is saved in a unique name
        _appDataFolder.rename(_appDataFolder.path(), _savedAppDataFolder.path());
    }

    // Copy an "empty" AppData folder (i.e. no entities)
    QDir canonicalAppDataFolder;
#ifdef Q_OS_WIN
    canonicalAppDataFolder = QDir::currentPath() + "/AppDataHighFidelity";
#elif defined Q_OS_MAC
    canonicalAppDataFolder = QCoreApplication::applicationDirPath() + "/AppDataHighFidelity";
#endif
    if (canonicalAppDataFolder.exists()) {
        copyFolder(canonicalAppDataFolder.path(), _appDataFolder.path());
    } else {
        QMessageBox::critical(0, "Internal error", "The nitpick AppData folder cannot be found at:\n" + canonicalAppDataFolder.path());
        exit(-1);
    }
}

void TestRunnerDesktop::createSnapshotFolder() {
    _snapshotFolder = _workingFolder + "/" + SNAPSHOT_FOLDER_NAME;

    // Just delete all PNGs from the folder if it already exists
    if (QDir(_snapshotFolder).exists()) {
        // Note that we cannot use just a `png` filter, as the filenames include periods
        // Also, delete any `jpg` and `txt` files
        // The idea is to leave only previous zipped result folders
        QDirIterator it(_snapshotFolder);
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

void TestRunnerDesktop::killProcesses() {
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

void TestRunnerDesktop::startLocalServerProcesses() {
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

void TestRunnerDesktop::runInterfaceWithTestScript() {
    QString url = QString("hifi://localhost");
    if (_runServerless->isChecked()) {
        // Move to an empty area
        url = "file:///~serverless/tutorial.json";
    } else {
        url = "hifi://localhost";
    }

    QString deleteScript =
        QString("https://raw.githubusercontent.com/") + _user + "/hifi_tests/" + _branch + "/tests/utils/deleteNearbyEntities.js";

    QString testScript = (_runFullSuite->isChecked())
        ? QString("https://raw.githubusercontent.com/") + _user + "/hifi_tests/" + _branch + "/tests/testRecursive.js"
        : _scriptURL->text();

    QString commandLine;
#ifdef Q_OS_WIN
    QString exeFile;
    // First, run script to delete any entities in test area
    // Note that this will run to completion before continuing
    exeFile = QString("\"") + QDir::toNativeSeparators(_installationFolder) + "\\interface.exe\"";
    commandLine = "start /wait \"\" " + exeFile +
        " --url " + url +
        " --no-updater" +
        " --no-login-suggestion"
        " --testScript " + deleteScript + " quitWhenFinished";

    system(commandLine.toStdString().c_str());

    // Now run the test suite
    exeFile = QString("\"") + QDir::toNativeSeparators(_installationFolder) + "\\interface.exe\"";
    commandLine = exeFile +
    " --url " + url +
    " --no-updater" +
    " --no-login-suggestion"
    " --testScript " + testScript + " quitWhenFinished" +
    " --testResultsLocation " + _snapshotFolder;

    _interfaceWorker->setCommandLine(commandLine);
    emit startInterface();
#elif defined Q_OS_MAC
    QFile script;
    script.setFileName(_workingFolder + "/runInterfaceTests.sh");
    if (!script.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not open 'runInterfaceTests.sh'");
        exit(-1);
    }

    script.write("#!/bin/sh\n\n");

    // First, run script to delete any entities in test area
    commandLine =
    "open -W \"" +_installationFolder + "/interface.app\" --args" +
    " --url " + url +
    " --no-updater" +
    " --no-login-suggestion"
    " --testScript " + deleteScript + " quitWhenFinished\n";

    script.write(commandLine.toStdString().c_str());

    // On The Mac, we need to resize Interface.  The Interface window opens a few seconds after the process
    // has started.
    // Before starting interface, start a process that will resize interface 10s after it opens
    commandLine = _workingFolder +"/waitForStart.sh interface && sleep 10 && " + _workingFolder +"/setInterfaceSizeAndPosition.sh &\n";
    script.write(commandLine.toStdString().c_str());

    commandLine =
        "open \"" +_installationFolder + "/interface.app\" --args" +
        " --url " + url +
        " --no-updater" +
        " --no-login-suggestion"
        " --testScript " + testScript + " quitWhenFinished" +
        " --testResultsLocation " + _snapshotFolder +
        " && " + _workingFolder +"/waitForFinish.sh interface\n";

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

void TestRunnerDesktop::interfaceExecutionComplete() {
    QThread::msleep(500);
    QFileInfo testCompleted(QDir::toNativeSeparators(_snapshotFolder) +"/tests_completed.txt");
    if (!testCompleted.exists()) {
        QMessageBox::critical(0, "Tests not completed", "Interface seems to have crashed before completion of the test scripts\nExisting images will be evaluated");
    }

    killProcesses();

    evaluateResults();

    // The Vircadia AppData folder will be restored after evaluation has completed
}

void TestRunnerDesktop::evaluateResults() {
    _statusLabel->setText("Evaluating results");
    nitpick->startTestsEvaluation(false, true, _snapshotFolder, _branch, _user);
}

void TestRunnerDesktop::automaticTestRunEvaluationComplete(const QString& zippedFolder, int numberOfFailures) {
    addBuildNumberToResults(zippedFolder);
    restoreHighFidelityAppDataFolder();

    _statusLabel->setText("Testing complete");

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

void TestRunnerDesktop::addBuildNumberToResults(const QString& zippedFolderName) {
    QString augmentedFilename { zippedFolderName };
    if (!_runLatest->isChecked()) {
        augmentedFilename.replace("local", getPRNumberFromURL(_url->text()));
    } else {
        augmentedFilename.replace("local", _buildInformation.build);
    }

    if (!QFile::rename(zippedFolderName, augmentedFilename)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Could not rename '" + zippedFolderName + "' to '" + augmentedFilename);
        exit(-1);
    }
}

void TestRunnerDesktop::restoreHighFidelityAppDataFolder() {
    _appDataFolder.removeRecursively();

    if (_savedAppDataFolder != QDir()) {
        _appDataFolder.rename(_savedAppDataFolder.path(), _appDataFolder.path());
    }
}

// Copies a folder recursively
void TestRunnerDesktop::copyFolder(const QString& source, const QString& destination) {
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

void TestRunnerDesktop::checkTime() {
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

QString TestRunnerDesktop::getPRNumberFromURL(const QString& url) {
    try {
        QStringList urlParts = url.split("/");
        if (urlParts.size() <= 2) {
#ifdef Q_OS_WIN
            throw "URL not in expected format, should look like `https://deployment.highfidelity.com/jobs/pr-build/label%3Dwindows/13023/HighFidelity-Beta-Interface-PR14006-be76c43.exe`";
#elif defined Q_OS_MAC
            throw "URL not in expected format, should look like `https://deployment.highfidelity.com/jobs/pr-build/label%3Dwindows/13023/HighFidelity-Beta-Interface-PR14006-be76c43.dmg`";
#endif
        }
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
