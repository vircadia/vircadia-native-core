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
#include <QSignalMapper>
#include <QTextEdit>
#include "ui_Nitpick.h"

#include "../Downloader.h"
#include "../Test.h"

#include "HelpWindow.h"
#include "../TestRunner.h"
#include "../AWSInterface.h"

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

    void downloadFile(const QUrl& url);
    void downloadFiles(const QStringList& URLs, const QString& directoryName, const QStringList& filenames, void* caller);

    void setUserText(const QString& user);
    QString getSelectedUser();

    void setBranchText(const QString& branch);
    QString getSelectedBranch();

    void enableRunTabControls();

    void updateStatusLabel(const QString& status);
    void appendLogWindow(const QString& message);

private slots:
    void on_tabWidget_currentChanged(int index);

    void on_evaluateTestsButton_clicked();
    void on_createRecursiveScriptButton_clicked();
    void on_createAllRecursiveScriptsButton_clicked();
    void on_createTestsButton_clicked();

    void on_createMDFileButton_clicked();
    void on_createAllMDFilesButton_clicked();

    void on_createTestAutoScriptButton_clicked();
    void on_createAllTestAutoScriptsButton_clicked();

    void on_createTestsOutlineButton_clicked();

    void on_createTestRailTestCasesButton_clicked();
    void on_createTestRailRunButton_clicked();

    void on_setWorkingFolderButton_clicked();
    void on_runNowButton_clicked();

    void on_checkBoxRunLatest_clicked();

    void on_updateTestRailRunResultsButton_clicked();

    void on_hideTaskbarButton_clicked();
    void on_showTaskbarButton_clicked();

    void on_createPythonScriptRadioButton_clicked();
    void on_createXMLScriptRadioButton_clicked();

    void on_createWebPagePushButton_clicked();

    void on_closeButton_clicked();

    void saveFile(int index);

    void about();
    void content();

private:
    Ui::NitpickClass _ui;
    Test* _test{ nullptr };
    TestRunner* _testRunner{ nullptr };

    AWSInterface _awsInterface;

    std::vector<Downloader*> _downloaders;

    // local storage for parameters - folder to store downloaded files in, and a list of their names
    QString _directoryName;
    QStringList _filenames;

    // Used to enable passing a parameter to slots
    QSignalMapper* _signalMapper;

    int _numberOfFilesToDownload{ 0 };
    int _numberOfFilesDownloaded{ 0 };
    int _index{ 0 };

    bool _isRunningFromCommandline{ false };

    HelpWindow _helpWindow;

    void* _caller;
};

#endif  // hifi_Nitpick_h