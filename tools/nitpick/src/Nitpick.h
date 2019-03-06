//
//  Nitpick.h
//
//  Created by Nissim Hadar on 2 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_Nitpick_h
#define hifi_Nitpick_h

#include <QtWidgets/QMainWindow>
#include <QTextEdit>
#include "ui_Nitpick.h"

#include "TestCreator.h"

#include "TestRunnerDesktop.h"
#include "TestRunnerMobile.h"

class Nitpick : public QMainWindow {
    Q_OBJECT

public:
    Nitpick(QWidget* parent = Q_NULLPTR);
    ~Nitpick();

    void setup();

    void startTestsEvaluation(const bool isRunningFromCommandLine,
                              const bool isRunningInAutomaticTestRun,
                              const QString& snapshotDirectory,
                              const QString& branch,
                              const QString& user);

    void automaticTestRunEvaluationComplete(QString zippedFolderName, int numberOfFailures);

    void setUserText(const QString& user);
    QString getSelectedUser();

    void setBranchText(const QString& branch);
    QString getSelectedBranch();

    void enableRunTabControls();

    void appendLogWindow(const QString& message);

private slots:
    void on_closePushbutton_clicked();

    void on_tabWidget_currentChanged(int index);

    void on_createRecursiveScriptPushbutton_clicked();
    void on_createAllRecursiveScriptsPushbutton_clicked();
    void on_createTestsPushbutton_clicked();

    void on_createMDFilePushbutton_clicked();
    void on_createAllMDFilesPushbutton_clicked();

    void on_createTestAutoScriptPushbutton_clicked();
    void on_createAllTestAutoScriptsPushbutton_clicked();

    void on_createTestsOutlinePushbutton_clicked();

    void on_createTestRailTestCasesPushbutton_clicked();
    void on_createTestRailRunButton_clicked();

    void on_setWorkingFolderRunOnDesktopPushbutton_clicked();
    void on_runNowPushbutton_clicked();

    void on_usePreviousInstallationOnDesktopCheckBox_clicked();
    void on_runLatestOnDesktopCheckBox_clicked();
    void on_runFullSuiteOnDesktopCheckBox_clicked();

    void on_updateTestRailRunResultsPushbutton_clicked();

    void on_hideTaskbarPushbutton_clicked();
    void on_showTaskbarPushbutton_clicked();

    void on_evaluateTestsPushbutton_clicked();

    void on_createPythonScriptRadioButton_clicked();
    void on_createXMLScriptRadioButton_clicked();

    void on_createWebPagePushbutton_clicked();

    void about();
    void content();

    // Run on Mobile controls
    void on_setWorkingFolderRunOnMobilePushbutton_clicked();
    void on_connectDevicePushbutton_clicked();

    void on_usePreviousInstallationOnMobileCheckBox_clicked();
    void on_runLatestOnMobileCheckBox_clicked();
    void on_runFullSuiteOnMobileCheckBox_clicked();

    void on_downloadAPKPushbutton_clicked();
    void on_installAPKPushbutton_clicked();
    void on_runInterfacePushbutton_clicked();

    void on_pullFolderPushbutton_clicked();

private:
    Ui::NitpickClass _ui;
    TestCreator* _testCreator{ nullptr };

    TestRunnerDesktop* _testRunnerDesktop{ nullptr };
    TestRunnerMobile* _testRunnerMobile{ nullptr };

    bool _isRunningFromCommandline{ false };

    QStringList clientProfiles;
};

#endif  // hifi_Nitpick_h