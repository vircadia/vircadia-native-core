//
//  TestRunner.h
//
//  Created by Nissim Hadar on 23 Jan 2019.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_testRunner_h
#define hifi_testRunner_h

#include "Downloader.h"

#include <QCheckBox>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QTimeEdit>

class Worker;

class BuildInformation {
public:
    QString build;
    QString url;
};

class TestRunner {
public:
    TestRunner(
        QLabel* workingFolderLabel,
        QLabel* statusLabel,
        QCheckBox* usePreviousInstallationOnMobileCheckBox,
        QCheckBox* runLatest,
        QLineEdit* url,
        QCheckBox* runFullSuite,
        QLineEdit* scriptURL
    );

    void setWorkingFolder(QLabel* workingFolderLabel);
 
    void downloadBuildXml(void* caller);
    void parseBuildInformation();
    QString getInstallerNameFromURL(const QString& url);

    void appendLog(const QString& message);

protected:
    Downloader* _downloader;

    QLabel* _workingFolderLabel;
    QLabel* _statusLabel;
    QCheckBox* _usePreviousInstallationCheckBox;
    QCheckBox* _runLatest;
    QLineEdit* _url;
    QCheckBox* _runFullSuite;
    QLineEdit* _scriptURL;

    QString _workingFolder;

    const QString DEV_BUILD_XML_URL{ "https://metaverse.highfidelity.com/dev-builds.xml" };
    const QString DEV_BUILD_XML_FILENAME{ "dev-builds.xml" };

    bool buildXMLDownloaded;
    BuildInformation _buildInformation;

#ifdef Q_OS_WIN
    const QString INSTALLER_FILENAME_LATEST{ "HighFidelity-Beta-latest-dev.exe" };
#elif defined(Q_OS_MAC)
    const QString INSTALLER_FILENAME_LATEST{ "HighFidelity-Beta-latest-dev.dmg" };
#else
    const QString INSTALLER_FILENAME_LATEST{ "" };
#endif

    QDateTime _testStartDateTime;

private:
    QFile _logFile;
};

class Worker : public QObject {
    Q_OBJECT
public:
    void setCommandLine(const QString& commandLine);

public slots:
    int runCommand();

signals:
    void commandComplete();

private:
    QString _commandLine;
};

#endif
