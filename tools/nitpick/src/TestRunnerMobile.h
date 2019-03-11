//
//  TestRunnerMobile.h
//
//  Created by Nissim Hadar on 22 Jan 2019.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_testRunnerMobile_h
#define hifi_testRunnerMobile_h

#include <QMap>
#include <QObject>
#include <QPushButton>

#include "TestRunner.h"
#include "AdbInterface.h"

class TestRunnerMobile : public QObject, public TestRunner {
    Q_OBJECT
public:
    explicit TestRunnerMobile(
        QLabel* workingFolderLabel,
        QPushButton *connectDeviceButton,
        QPushButton *pullFolderButton,
        QLabel* detectedDeviceLabel,
        QLineEdit *folderLineEdit,
        QPushButton* downloadAPKPushbutton,
        QPushButton* installAPKPushbutton,
        QPushButton* runInterfacePushbutton,
        QCheckBox* usePreviousInstallationOnMobileCheckBox,
        QCheckBox* runLatest,
        QLineEdit* url,
        QCheckBox* runFullSuite,
        QLineEdit* scriptURL,
        QLabel* statusLabel,

        QObject* parent = 0
    );
    ~TestRunnerMobile();

    void setWorkingFolderAndEnableControls();
    void connectDevice();

    void downloadComplete();
    void downloadAPK();
    void runInterface();

    void installAPK();

    void pullFolder();

private:
    QPushButton* _connectDeviceButton;
    QPushButton* _pullFolderButton;
    QLabel* _detectedDeviceLabel;
    QLineEdit* _folderLineEdit;
    QPushButton* _downloadAPKPushbutton;
    QPushButton* _installAPKPushbutton;
    QPushButton* _runInterfacePushbutton;

#ifdef Q_OS_WIN
    const QString _adbExe{ "adb.exe" };
#else
    // Both Mac and Linux use "adb"
    const QString _adbExe{ "adb" };
#endif

    QString _installerFilename;

    QString _adbCommand;

    std::map<QString, QString> modelNames;

    AdbInterface* _adbInterface;

    QString _modelName;
};
#endif
