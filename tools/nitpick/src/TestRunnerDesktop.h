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

#include <QDir>
#include <QLabel>
#include <QObject>
#include <QPushButton>
#include <QThread>
#include <QTimer>

#include "TestRunner.h"

class InterfaceWorker;
class InstallerWorker;

class TestRunnerDesktop : public QObject, public TestRunner {
    Q_OBJECT
public:
    explicit TestRunnerDesktop(
        std::vector<QCheckBox*> dayCheckboxes,
        std::vector<QCheckBox*> timeEditCheckboxes,
        std::vector<QTimeEdit*> timeEdits,
        QLabel* workingFolderLabel,
        QCheckBox* runServerless,
        QCheckBox* runLatest,
        QLineEdit* url,
        QPushButton* runNow,
        QLabel* statusLabel,

        QObject* parent = 0
    );

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
    void automaticTestRunEvaluationComplete(const QString& zippedFolderName, int numberOfFailures);
    void addBuildNumberToResults(const QString& zippedFolderName);

    void copyFolder(const QString& source, const QString& destination);

    QString getPRNumberFromURL(const QString& url);

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

    QString _installerURL;
    QString _installerFilename;

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
    QPushButton* _runNow;
    QTimer* _timer;
    QThread* _installerThread;
    QThread* _interfaceThread;

    InstallerWorker* _installerWorker;
    InterfaceWorker* _interfaceWorker;
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
