//
//  TestRunnerDesktop.h
//
//  Created by Nissim Hadar on 1 Sept 2018.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_testRunnerDesktop_h
#define hifi_testRunnerDesktop_h

#include <QCheckBox>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include <QThread>
#include <QTimeEdit>
#include <QTimer>

#include "TestRunner.h"

class BuildInformation {
public:
    QString build;
    QString url;
};

class InterfaceWorker;
class InstallerWorker;

class TestRunnerDesktop : public QObject, public TestRunner {
    Q_OBJECT
public:
    explicit TestRunnerDesktop(std::vector<QCheckBox*> dayCheckboxes,
                        std::vector<QCheckBox*> timeEditCheckboxes,
                        std::vector<QTimeEdit*> timeEdits,
                        QLabel* workingFolderLabel,
                        QCheckBox* runServerless,
                        QCheckBox* runLatest,
                        QLineEdit* url,
                        QPushButton* runNow,
                        QObject* parent = 0);

    ~TestRunnerDesktop();

    void setWorkingFolderAndEnableControls();

    void run();

    void downloadComplete();
    void runInstaller();
    void verifyInstallationSucceeded();

    void saveExistingHighFidelityAppDataFolder();
    void restoreHighFidelityAppDataFolder();

    void createSnapshotFolder();
    
    void killProcesses();
    void startLocalServerProcesses();
    
    void runInterfaceWithTestScript();

    void evaluateResults();
    void automaticTestRunEvaluationComplete(QString zippedFolderName, int numberOfFailures);
    void addBuildNumberToResults(QString zippedFolderName);

    void copyFolder(const QString& source, const QString& destination);

    void updateStatusLabel(const QString& message);
    void appendLog(const QString& message);

    QString getInstallerNameFromURL(const QString& url);
    QString getPRNumberFromURL(const QString& url);

    void parseBuildInformation();

private slots:
    void checkTime();
    void installationComplete();
    void interfaceExecutionComplete();

signals:
    void startInstaller();
    void startInterface();
    void startResize();
    
private:
    bool _automatedTestIsRunning{ false };

#ifdef Q_OS_WIN
    const QString INSTALLER_FILENAME_LATEST{ "HighFidelity-Beta-latest-dev.exe" };
#elif defined(Q_OS_MAC)
    const QString INSTALLER_FILENAME_LATEST{ "HighFidelity-Beta-latest-dev.dmg" };
#else
    const QString INSTALLER_FILENAME_LATEST{ "" };
#endif

    QString _installerURL;
    QString _installerFilename;
    const QString DEV_BUILD_XML_URL{ "https://highfidelity.com/dev-builds.xml" };
    const QString DEV_BUILD_XML_FILENAME{ "dev-builds.xml" };

    bool buildXMLDownloaded;

    QDir _appDataFolder;
    QDir _savedAppDataFolder;

    QString _installationFolder;
    QString _snapshotFolder;

    const QString UNIQUE_FOLDER_NAME{ "fgadhcUDHSFaidsfh3478JJJFSDFIUSOEIrf" };
    const QString SNAPSHOT_FOLDER_NAME{ "snapshots" };

    QString _branch;
    QString _user;

    std::vector<QCheckBox*> _dayCheckboxes;
    std::vector<QCheckBox*> _timeEditCheckboxes;
    std::vector<QTimeEdit*> _timeEdits;
    QLabel* _workingFolderLabel;
    QCheckBox* _runServerless;
    QCheckBox* _runLatest;
    QLineEdit* _url;
    QPushButton* _runNow;
    QTimer* _timer;

    QFile _logFile;

    QDateTime _testStartDateTime;

    QThread* _installerThread;
    QThread* _interfaceThread;

    InstallerWorker* _installerWorker;
    InterfaceWorker* _interfaceWorker;

    BuildInformation _buildInformation;
};

class InstallerWorker : public Worker {
    Q_OBJECT
signals:
    void startInstaller();
};

class InterfaceWorker : public Worker {
    Q_OBJECT
signals:
    void startInterface();
};
#endif
