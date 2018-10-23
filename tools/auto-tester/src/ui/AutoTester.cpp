//
//  AutoTester.cpp
//  zone/ambientLightInheritence
//
//  Created by Nissim Hadar on 2 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "AutoTester.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

AutoTester::AutoTester(QWidget* parent) : QMainWindow(parent) {
    _ui.setupUi(this);

    _ui.checkBoxInteractiveMode->setChecked(true);
    _ui.progressBar->setVisible(false);
    _ui.tabWidget->setCurrentIndex(0);

    _signalMapper = new QSignalMapper();

    connect(_ui.actionClose, &QAction::triggered, this, &AutoTester::on_closeButton_clicked);
    connect(_ui.actionAbout, &QAction::triggered, this, &AutoTester::about);
    connect(_ui.actionContent, &QAction::triggered, this, &AutoTester::content);

    // The second tab hides and shows the Windows task bar
#ifndef Q_OS_WIN
    _ui.tabWidget->removeTab(1);
#endif

   _ui.statusLabel->setText("");
   _ui.plainTextEdit->setReadOnly(true);

   setWindowTitle("Auto Tester - v6.7");

   // Coming soon to an auto-tester near you...
   //// _helpWindow.textBrowser->setText()
}

AutoTester::~AutoTester() {
    delete _signalMapper;

    if (_test) {
        delete _test;
    }

    if (_testRunner) {
        delete _testRunner;
    }
}

void AutoTester::setup() {
    if (_test) {
        delete _test;
    }
    _test = new Test(_ui.progressBar, _ui.checkBoxInteractiveMode);

    std::vector<QCheckBox*> dayCheckboxes;
    dayCheckboxes.emplace_back(_ui.mondayCheckBox);
    dayCheckboxes.emplace_back(_ui.tuesdayCheckBox);
    dayCheckboxes.emplace_back(_ui.wednesdayCheckBox);
    dayCheckboxes.emplace_back(_ui.thursdayCheckBox);
    dayCheckboxes.emplace_back(_ui.fridayCheckBox);
    dayCheckboxes.emplace_back(_ui.saturdayCheckBox);
    dayCheckboxes.emplace_back(_ui.sundayCheckBox);

    std::vector<QCheckBox*> timeEditCheckboxes;
    timeEditCheckboxes.emplace_back(_ui.timeEdit1checkBox);
    timeEditCheckboxes.emplace_back(_ui.timeEdit2checkBox);
    timeEditCheckboxes.emplace_back(_ui.timeEdit3checkBox);
    timeEditCheckboxes.emplace_back(_ui.timeEdit4checkBox);

    std::vector<QTimeEdit*> timeEdits;
    timeEdits.emplace_back(_ui.timeEdit1);
    timeEdits.emplace_back(_ui.timeEdit2);
    timeEdits.emplace_back(_ui.timeEdit3);
    timeEdits.emplace_back(_ui.timeEdit4);

    if (_testRunner) {
        delete _testRunner;
    }
    _testRunner = new TestRunner(dayCheckboxes, timeEditCheckboxes, timeEdits, _ui.workingFolderLabel, _ui.checkBoxServerless, _ui.checkBoxRunLatest, _ui.urlLineEdit, _ui.runNowButton);
}

void AutoTester::startTestsEvaluation(const bool isRunningFromCommandLine,
                                      const bool isRunningInAutomaticTestRun,
                                      const QString& snapshotDirectory,
                                      const QString& branch,
                                      const QString& user
) {
    _test->startTestsEvaluation(isRunningFromCommandLine, isRunningInAutomaticTestRun, snapshotDirectory, branch, user);
}

void AutoTester::on_tabWidget_currentChanged(int index) {
    if (index == 0 || index == 2 || index == 3) {
        _ui.userLineEdit->setDisabled(false);
        _ui.branchLineEdit->setDisabled(false);
    } else {
        _ui.userLineEdit->setDisabled(true);
        _ui.branchLineEdit->setDisabled(true);
    }
}

void AutoTester::on_evaluateTestsButton_clicked() {
    _test->startTestsEvaluation(false, false);
}

void AutoTester::on_createRecursiveScriptButton_clicked() {
    _test->createRecursiveScript();
}

void AutoTester::on_createAllRecursiveScriptsButton_clicked() {
    _test->createAllRecursiveScripts();
}

void AutoTester::on_createTestsButton_clicked() {
    _test->createTests();
}

void AutoTester::on_createMDFileButton_clicked() {
    _test->createMDFile();
}

void AutoTester::on_createAllMDFilesButton_clicked() {
    _test->createAllMDFiles();
}

void AutoTester::on_createTestAutoScriptButton_clicked() {
    _test->createTestAutoScript();
}

void AutoTester::on_createAllTestAutoScriptsButton_clicked() {
    _test->createAllTestAutoScripts();
}

void AutoTester::on_createTestsOutlineButton_clicked() {
    _test->createTestsOutline();
}

void AutoTester::on_createTestRailTestCasesButton_clicked() {
    _test->createTestRailTestCases();
}

void AutoTester::on_createTestRailRunButton_clicked() {
    _test->createTestRailRun();
}

void AutoTester::on_setWorkingFolderButton_clicked() {
    _testRunner->setWorkingFolder();
}

void AutoTester::enableRunTabControls() {
    _ui.runNowButton->setEnabled(true);
    _ui.daysGroupBox->setEnabled(true);
    _ui.timesGroupBox->setEnabled(true);
}

void AutoTester::on_runNowButton_clicked() {
    _testRunner->run();
}

void AutoTester::on_checkBoxRunLatest_clicked() {
    _ui.urlLineEdit->setEnabled(!_ui.checkBoxRunLatest->isChecked());
}

void AutoTester::automaticTestRunEvaluationComplete(QString zippedFolderName, int numberOfFailures) {
    _testRunner->automaticTestRunEvaluationComplete(zippedFolderName, numberOfFailures);
}

void AutoTester::on_updateTestRailRunResultsButton_clicked() {
    _test->updateTestRailRunResult();
}

// To toggle between show and hide
//   if (uState & ABS_AUTOHIDE) on_showTaskbarButton_clicked();
//   else on_hideTaskbarButton_clicked();
//
void AutoTester::on_hideTaskbarButton_clicked() {
#ifdef Q_OS_WIN
    APPBARDATA abd = { sizeof abd };
    UINT uState = (UINT)SHAppBarMessage(ABM_GETSTATE, &abd);
    LPARAM param = uState & ABS_ALWAYSONTOP;
    abd.lParam = ABS_AUTOHIDE | param;
    SHAppBarMessage(ABM_SETSTATE, &abd);
#endif
}

void AutoTester::on_showTaskbarButton_clicked() {
#ifdef Q_OS_WIN
    APPBARDATA abd = { sizeof abd };
    UINT uState = (UINT)SHAppBarMessage(ABM_GETSTATE, &abd);
    LPARAM param = uState & ABS_ALWAYSONTOP;
    abd.lParam = param;
    SHAppBarMessage(ABM_SETSTATE, &abd);
#endif
}

void AutoTester::on_closeButton_clicked() {
    exit(0);
}

void AutoTester::on_createPythonScriptRadioButton_clicked() {
    _test->setTestRailCreateMode(PYTHON);
}

void AutoTester::on_createXMLScriptRadioButton_clicked() {
    _test->setTestRailCreateMode(XML);
}

void AutoTester::on_createWebPagePushButton_clicked() {
    _test->createWebPage(_ui.updateAWSCheckBox, _ui.awsURLLineEdit);
}

void AutoTester::downloadFile(const QUrl& url) {
    _downloaders.emplace_back(new Downloader(url, this));
    connect(_downloaders[_index], SIGNAL(downloaded()), _signalMapper, SLOT(map()));

    _signalMapper->setMapping(_downloaders[_index], _index);

    ++_index;
}

void AutoTester::downloadFiles(const QStringList& URLs, const QString& directoryName, const QStringList& filenames, void *caller) {
    connect(_signalMapper, SIGNAL(mapped(int)), this, SLOT(saveFile(int)));
    
    _directoryName = directoryName;
    _filenames = filenames;
    _caller = caller;

    _numberOfFilesToDownload = URLs.size();
    _numberOfFilesDownloaded = 0;
    _index = 0;

    _ui.progressBar->setMinimum(0);
    _ui.progressBar->setMaximum(_numberOfFilesToDownload - 1);
    _ui.progressBar->setValue(0);
    _ui.progressBar->setVisible(true);

    foreach (auto downloader, _downloaders) {
        delete downloader;
    }

    _downloaders.clear();
    for (int i = 0; i < _numberOfFilesToDownload; ++i) {
        downloadFile(URLs[i]);
    }
}

void AutoTester::saveFile(int index) {
    try {
        QFile file(_directoryName + "/" + _filenames[index]);
        file.open(QIODevice::WriteOnly);
        file.write(_downloaders[index]->downloadedData());
        file.close();
    } catch (...) {
        QMessageBox::information(0, "Test Aborted", "Failed to save file: " + _filenames[index]);
        _ui.progressBar->setVisible(false);
        return;
    }

    ++_numberOfFilesDownloaded;

    if (_numberOfFilesDownloaded == _numberOfFilesToDownload) {
        disconnect(_signalMapper, SIGNAL(mapped(int)), this, SLOT(saveFile(int)));
        if (_caller == _test) {
            _test->finishTestsEvaluation();
        } else if (_caller == _testRunner) {
            _testRunner->downloadComplete();
        }
    } else {
        _ui.progressBar->setValue(_numberOfFilesDownloaded);
    }
}

void AutoTester::about() {
    QMessageBox::information(0, "About", QString("Built ") + __DATE__ + ", " + __TIME__);
}

void AutoTester::content() {
    _helpWindow.show();
}

void AutoTester::setUserText(const QString& user) {
    _ui.userLineEdit->setText(user);
}

QString AutoTester::getSelectedUser() {
    return _ui.userLineEdit->text();
}

void AutoTester::setBranchText(const QString& branch) {
    _ui.branchLineEdit->setText(branch);
}

QString AutoTester::getSelectedBranch() {
    return _ui.branchLineEdit->text();
}

void AutoTester::updateStatusLabel(const QString& status) {
    _ui.statusLabel->setText(status);
}

void AutoTester::appendLogWindow(const QString& message) {
    _ui.plainTextEdit->appendPlainText(message);
}