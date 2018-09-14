//
//  Downloader.h
//
//  Created by Nissim Hadar on 1 Sept 2018.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_testRunner_h
#define hifi_testRunner_h

#include <QCheckBox>
#include <QDir>
#include <QLabel>
#include <QObject>
#include <QThread>
#include <QTimeEdit>
#include <QTimer>

class Worker;

class TestRunner : public QObject {
    Q_OBJECT
public:
    explicit TestRunner(std::vector<QCheckBox*> dayCheckboxes,
                        std::vector<QCheckBox*> timeEditCheckboxes,
                        std::vector<QTimeEdit*> timeEdits,
                        QLabel* workingFolderLabel,
                        QObject* parent = 0);

    void setWorkingFolder();

    void run();

    void installerDownloadComplete();
    void runInstaller();

    void saveExistingHighFidelityAppDataFolder();
    void restoreHighFidelityAppDataFolder();

    void createSnapshotFolder();
    
    void killProcesses();
    void startLocalServerProcesses();
    
    void runInterfaceWithTestScript();

    void evaluateResults();
    void automaticTestRunEvaluationComplete(QString zippedFolderName);
    void addBuildNumberToResults(QString zippedFolderName);

    void copyFolder(const QString& source, const QString& destination);

    void updateStatusLabel(const QString& message);
    void appendLog(const QString& message);

private slots:
    void checkTime();
    void installationComplete();
    void interfaceExecutionComplete();

private:
    bool _automatedTestIsRunning{ false };

    const QString INSTALLER_URL{ "http://builds.highfidelity.com/HighFidelity-Beta-latest-dev.exe" };
    const QString INSTALLER_FILENAME{ "HighFidelity-Beta-latest-dev.exe" };

    const QString BUILD_XML_URL{ "https://highfidelity.com/dev-builds.xml" };
    const QString BUILD_XML_FILENAME{ "dev-builds.xml" };

    QDir _appDataFolder;
    QDir _savedAppDataFolder;

    QString _workingFolder;
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

    QTimer* _timer;

    QFile _logFile;

    QDateTime _testStartDateTime;

    QThread* installerThread;
    QThread* interfaceThread;
    Worker* worker;
};

class Worker : public QObject {
    Q_OBJECT
public:
    Worker(const QString commandLine);
public slots:
    void process();

signals:
    void finished();

private:
    QString _commandLine;
};
#endif  // hifi_testRunner_h