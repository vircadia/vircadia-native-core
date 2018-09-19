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
#include <QTextEdit>
#include <QThread>
#include <QTimeEdit>
#include <QTimer>

class Worker;

class Runner;

class TestRunner : public QObject {
    Q_OBJECT
public:
    explicit TestRunner(std::vector<QCheckBox*> dayCheckboxes,
                        std::vector<QCheckBox*> timeEditCheckboxes,
                        std::vector<QTimeEdit*> timeEdits,
                        QLabel* workingFolderLabel,
                        QCheckBox* runServerless,
                        QCheckBox* runLatest,
                        QTextEdit* url,
                        QObject* parent = 0);

    ~TestRunner();

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
    void automaticTestRunEvaluationComplete(QString zippedFolderName, int numberOfFailures);
    void addBuildNumberToResults(QString zippedFolderName);

    void copyFolder(const QString& source, const QString& destination);

    void appendLog(const QString& message);

    QString getInstallerNameFromURL(const QString& url);
    QString getPRNumberFromURL(const QString& url);

private slots:
    void checkTime();
    void installationComplete();
    void interfaceExecutionComplete();

signals:
    void startInstaller();
    void startInterface();

private:
    bool _automatedTestIsRunning{ false };

    const QString INSTALLER_URL_LATEST{ "http://builds.highfidelity.com/HighFidelity-Beta-latest-dev.exe" };
    const QString INSTALLER_FILENAME_LATEST{ "HighFidelity-Beta-latest-dev.exe" };

    QString _installerURL;
    QString _installerFilename;
    const QString BUILD_XML_URL{ "https://highfidelity.com/dev-builds.xml" };
    const QString BUILD_XML_FILENAME{ "dev-builds.xml" };

    QDir _appDataFolder;
    QDir _savedAppDataFolder;

<<<<<<< HEAD
    QString _snapshotFolder;

=======
    QString _workingFolder;
>>>>>>> 8301b472feeaf857d4273f65eedb97957bc13755
    QString _installationFolder;
    QString _snapshotFolder;

    const QString UNIQUE_FOLDER_NAME{ "fgadhcUDHSFaidsfh3478JJJFSDFIUSOEIrf" };
    const QString SNAPSHOT_FOLDER_NAME{ "snapshots" };

    QString _branch;
    QString _user;

    std::vector<QCheckBox*> _dayCheckboxes;
    std::vector<QCheckBox*> _timeEditCheckboxes;
    std::vector<QTimeEdit*> _timeEdits;
<<<<<<< HEAD
=======
    QLabel* _workingFolderLabel;
    QCheckBox* _runServerless;
    QCheckBox* _runLatest;
    QTextEdit* _url;
>>>>>>> 8301b472feeaf857d4273f65eedb97957bc13755

    QTimer* _timer;

    QFile _logFile;
    Runner* runner;
};

class Runner : public QObject {
    Q_OBJECT

public:
    friend TestRunner;

    Runner();
    ~Runner();

    void updateStatusLabel(const QString& message);

public slots:
    void process();

signals:
    void finished();
    void error(QString err);

private:
    QDateTime _testStartDateTime;

    QThread* installerThread;
    QThread* interfaceThread;
    Worker* installerWorker;
    Worker* interfaceWorker;
};

class Worker : public QObject {
    Q_OBJECT
public:
    void setCommandLine(const QString& commandLine);

public slots:
    void runCommand();

signals:
    void commandComplete();
    void startInstaller();
    void startInterface();

private:
    QString _commandLine;
};
#endif  // hifi_testRunner_h