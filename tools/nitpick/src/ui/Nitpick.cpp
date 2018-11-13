//
//  Nitpick.cpp
//  zone/ambientLightInheritence
//
//  Created by Nissim Hadar on 2 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Nitpick.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

Nitpick::Nitpick(QWidget* parent) : QMainWindow(parent) {
    _ui.setupUi(this);

    _ui.checkBoxInteractiveMode->setChecked(true);
    _ui.progressBar->setVisible(false);
    _ui.tabWidget->setCurrentIndex(0);

    _signalMapper = new QSignalMapper();

    connect(_ui.actionClose, &QAction::triggered, this, &Nitpick::on_closeButton_clicked);
    connect(_ui.actionAbout, &QAction::triggered, this, &Nitpick::about);
    connect(_ui.actionContent, &QAction::triggered, this, &Nitpick::content);

    // The second tab hides and shows the Windows task bar
#ifndef Q_OS_WIN
    _ui.tabWidget->removeTab(1);
#endif

   _ui.statusLabel->setText("");
   _ui.plainTextEdit->setReadOnly(true);

   setWindowTitle("Nitpick - v1.0");

   // Coming soon to a nitpick near you...
   //// _helpWindow.textBrowser->setText()
}

Nitpick::~Nitpick() {
    delete _signalMapper;

    if (_test) {
        delete _test;
    }

    if (_testRunner) {
        delete _testRunner;
    }
}

void Nitpick::setup() {
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

void Nitpick::startTestsEvaluation(const bool isRunningFromCommandLine,
                                      const bool isRunningInAutomaticTestRun,
                                      const QString& snapshotDirectory,
                                      const QString& branch,
                                      const QString& user
) {
    _test->startTestsEvaluation(isRunningFromCommandLine, isRunningInAutomaticTestRun, snapshotDirectory, branch, user);
}

void Nitpick::on_tabWidget_currentChanged(int index) {
// Enable the GitHub edit boxes as required
#ifdef Q_OS_WIN
    if (index == 0 || index == 2 || index == 3) {
#else
    if (index == 0 || index == 1 || index == 2) {
#endif
        _ui.userLineEdit->setDisabled(false);
        _ui.branchLineEdit->setDisabled(false);
    } else {
        _ui.userLineEdit->setDisabled(true);
        _ui.branchLineEdit->setDisabled(true);
    }
}

void Nitpick::on_evaluateTestsButton_clicked() {
    _test->startTestsEvaluation(false, false);
}

void Nitpick::on_createRecursiveScriptButton_clicked() {
    _test->createRecursiveScript();
}

void Nitpick::on_createAllRecursiveScriptsButton_clicked() {
    _test->createAllRecursiveScripts();
}

void Nitpick::on_createTestsButton_clicked() {
    _test->createTests();
}

void Nitpick::on_createMDFileButton_clicked() {
    _test->createMDFile();
}

void Nitpick::on_createAllMDFilesButton_clicked() {
    _test->createAllMDFiles();
}

void Nitpick::on_createTestAutoScriptButton_clicked() {
    _test->createTestAutoScript();
}

void Nitpick::on_createAllTestAutoScriptsButton_clicked() {
    _test->createAllTestAutoScripts();
}

void Nitpick::on_createTestsOutlineButton_clicked() {
    _test->createTestsOutline();
}

void Nitpick::on_createTestRailTestCasesButton_clicked() {
    _test->createTestRailTestCases();
}

void Nitpick::on_createTestRailRunButton_clicked() {
    _test->createTestRailRun();
}

void Nitpick::on_setWorkingFolderButton_clicked() {
    _testRunner->setWorkingFolder();
}

void Nitpick::enableRunTabControls() {
    _ui.runNowButton->setEnabled(true);
    _ui.daysGroupBox->setEnabled(true);
    _ui.timesGroupBox->setEnabled(true);
}

void Nitpick::on_runNowButton_clicked() {
    _testRunner->run();
}

void Nitpick::on_checkBoxRunLatest_clicked() {
    _ui.urlLineEdit->setEnabled(!_ui.checkBoxRunLatest->isChecked());
}

void Nitpick::automaticTestRunEvaluationComplete(QString zippedFolderName, int numberOfFailures) {
    _testRunner->automaticTestRunEvaluationComplete(zippedFolderName, numberOfFailures);
}

void Nitpick::on_updateTestRailRunResultsButton_clicked() {
    _test->updateTestRailRunResult();
}

// To toggle between show and hide
//   if (uState & ABS_AUTOHIDE) on_showTaskbarButton_clicked();
//   else on_hideTaskbarButton_clicked();
//
void Nitpick::on_hideTaskbarButton_clicked() {
#ifdef Q_OS_WIN
    APPBARDATA abd = { sizeof abd };
    UINT uState = (UINT)SHAppBarMessage(ABM_GETSTATE, &abd);
    LPARAM param = uState & ABS_ALWAYSONTOP;
    abd.lParam = ABS_AUTOHIDE | param;
    SHAppBarMessage(ABM_SETSTATE, &abd);
#endif
}

void Nitpick::on_showTaskbarButton_clicked() {
#ifdef Q_OS_WIN
    APPBARDATA abd = { sizeof abd };
    UINT uState = (UINT)SHAppBarMessage(ABM_GETSTATE, &abd);
    LPARAM param = uState & ABS_ALWAYSONTOP;
    abd.lParam = param;
    SHAppBarMessage(ABM_SETSTATE, &abd);
#endif
}

void Nitpick::on_closeButton_clicked() {
    exit(0);
}

void Nitpick::on_createPythonScriptRadioButton_clicked() {
    _test->setTestRailCreateMode(PYTHON);
}

void Nitpick::on_createXMLScriptRadioButton_clicked() {
    _test->setTestRailCreateMode(XML);
}

void Nitpick::on_createWebPagePushButton_clicked() {
    _test->createWebPage(_ui.updateAWSCheckBox, _ui.awsURLLineEdit);
}

void Nitpick::downloadFile(const QUrl& url) {
    _downloaders.emplace_back(new Downloader(url, this));
    connect(_downloaders[_index], SIGNAL(downloaded()), _signalMapper, SLOT(map()));

    _signalMapper->setMapping(_downloaders[_index], _index);

    ++_index;
}

void Nitpick::downloadFiles(const QStringList& URLs, const QString& directoryName, const QStringList& filenames, void *caller) {
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

void Nitpick::saveFile(int index) {
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

void Nitpick::about() {
    QMessageBox::information(0, "About", QString("Built ") + __DATE__ + ", " + __TIME__);
}

void Nitpick::content() {
    _helpWindow.show();
}

void Nitpick::setUserText(const QString& user) {
    _ui.userLineEdit->setText(user);
}

QString Nitpick::getSelectedUser() {
    return _ui.userLineEdit->text();
}

void Nitpick::setBranchText(const QString& branch) {
    _ui.branchLineEdit->setText(branch);
}

QString Nitpick::getSelectedBranch() {
    return _ui.branchLineEdit->text();
}

void Nitpick::updateStatusLabel(const QString& status) {
    _ui.statusLabel->setText(status);
}

void Nitpick::appendLogWindow(const QString& message) {
    _ui.plainTextEdit->appendPlainText(message);
}
