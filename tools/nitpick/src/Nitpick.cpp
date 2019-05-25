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
#include "Platform.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

#include <QDesktopServices>

Nitpick::Nitpick(QWidget* parent) : QMainWindow(parent) {
    _ui.setupUi(this);

    _ui.checkBoxInteractiveMode->setChecked(true);
    _ui.progressBar->setVisible(false);
    _ui.tabWidget->setCurrentIndex(0);

    connect(_ui.actionClose, &QAction::triggered, this, &Nitpick::on_closePushbutton_clicked);
    connect(_ui.actionAbout, &QAction::triggered, this, &Nitpick::about);
    connect(_ui.actionContent, &QAction::triggered, this, &Nitpick::content);

    // The second tab hides and shows the Windows task bar
#ifndef Q_OS_WIN
    _ui.tabWidget->removeTab(1);
#endif

    _ui.statusLabelOnDesktop->setText("");
    _ui.statusLabelOnMobile->setText("");
 
    _ui.plainTextEdit->setReadOnly(true);

    setWindowTitle("Nitpick - " + nitpickVersion);

    _GPUVendors << "Nvidia" << "AMD";
    _ui.gpuVendorComboBox->insertItems(0, _GPUVendors);

    QString gpuVendor = Platform::getGraphicsCardType().toUpper();
    if (gpuVendor.contains("NVIDIA")) {
        _ui.gpuVendorComboBox->setCurrentIndex(0);
    } else {
        _ui.gpuVendorComboBox->setCurrentIndex(1);
    }
}

Nitpick::~Nitpick() {
    if (_testCreator) {
        delete _testCreator;
    }

    if (_testRunnerDesktop) {
        delete _testRunnerDesktop;
    }

    if (_testRunnerMobile) {
        delete _testRunnerMobile;
    }
}

void Nitpick::setup() {
    if (_testCreator) {
        delete _testCreator;
    }
    _testCreator = new TestCreator(_ui.progressBar, _ui.checkBoxInteractiveMode);

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

    // Create the two test runners
    if (_testRunnerDesktop) {
        delete _testRunnerDesktop;
    }
    _testRunnerDesktop = new TestRunnerDesktop(
        dayCheckboxes, 
        timeEditCheckboxes, 
        timeEdits, 
        _ui.workingFolderRunOnDesktopLabel, 
        _ui.checkBoxServerless,
        _ui.usePreviousInstallationOnDesktopCheckBox,
        _ui.runLatestOnDesktopCheckBox, 
        _ui.urlOnDesktopLineEdit, 
        _ui.runFullSuiteOnDesktopCheckBox,
        _ui.scriptURLOnDesktopLineEdit,
        _ui.runNowPushbutton,
        _ui.statusLabelOnDesktop
    );

    if (_testRunnerMobile) {
        delete _testRunnerMobile;
    }
    _testRunnerMobile = new TestRunnerMobile(
        _ui.workingFolderRunOnMobileLabel, 
        _ui.connectDevicePushbutton, 
        _ui.pullFolderPushbutton, 
        _ui.detectedDeviceLabel, 
        _ui.folderLineEdit,
        _ui.downloadAPKPushbutton,
        _ui.installAPKPushbutton,
        _ui.runInterfacePushbutton,
        _ui.usePreviousInstallationOnMobileCheckBox,
        _ui.runLatestOnMobileCheckBox,
        _ui.urlOnMobileLineEdit,
        _ui.runFullSuiteOnMobileCheckBox,
        _ui.scriptURLOnMobileLineEdit,
        _ui.statusLabelOnMobile
    );
}

void Nitpick::startTestsEvaluation(
    const bool isRunningFromCommandLine,
    const bool isRunningInAutomaticTestRun,
    const QString& snapshotDirectory,
    const QString& branch,
    const QString& user
) {
    _testCreator->startTestsEvaluation(_ui.gpuVendorComboBox, isRunningFromCommandLine, isRunningInAutomaticTestRun, snapshotDirectory, branch, user);
}

void Nitpick::on_tabWidget_currentChanged(int index) {
// Enable the GitHub edit boxes as required
#ifdef Q_OS_WIN
    if (index == 0 || index == 2 || index == 3 || index == 4) {
#else
    if (index == 0 || index == 1 || index == 2 || index == 3) {
#endif
        _ui.userLineEdit->setDisabled(false);
        _ui.branchLineEdit->setDisabled(false);
        _ui.gpuVendorComboBox->setDisabled(false);
    } else {
        _ui.userLineEdit->setDisabled(true);
        _ui.branchLineEdit->setDisabled(true);
        _ui.gpuVendorComboBox->setDisabled(true);
    }
}

void Nitpick::on_createRecursiveScriptPushbutton_clicked() {
    _testCreator->createRecursiveScript();
}

void Nitpick::on_createAllRecursiveScriptsPushbutton_clicked() {
    _testCreator->createAllRecursiveScripts();
}

void Nitpick::on_createTestsPushbutton_clicked() {
    _testCreator->createTests(_ui.gpuVendorComboBox->currentText());
}

void Nitpick::on_createMDFilePushbutton_clicked() {
    _testCreator->createMDFile();
}

void Nitpick::on_createAllMDFilesPushbutton_clicked() {
    _testCreator->createAllMDFiles();
}

void Nitpick::on_createTestAutoScriptPushbutton_clicked() {
    _testCreator->createTestAutoScript();
}

void Nitpick::on_createAllTestAutoScriptsPushbutton_clicked() {
    _testCreator->createAllTestAutoScripts();
}

void Nitpick::on_createTestsOutlinePushbutton_clicked() {
    _testCreator->createTestsOutline();
}

void Nitpick::on_createTestRailTestCasesPushbutton_clicked() {
    _testCreator->createTestRailTestCases();
}

void Nitpick::on_createTestRailRunButton_clicked() {
    _testCreator->createTestRailRun();
}

void Nitpick::on_setWorkingFolderRunOnDesktopPushbutton_clicked() {
    _testRunnerDesktop->setWorkingFolderAndEnableControls();
}

void Nitpick::enableRunTabControls() {
    _ui.runNowPushbutton->setEnabled(true);
    _ui.daysGroupBox->setEnabled(true);
    _ui.timesGroupBox->setEnabled(true);
}

void Nitpick::on_runNowPushbutton_clicked() {
    _testRunnerDesktop->run();
}

void Nitpick::on_usePreviousInstallationOnDesktopCheckBox_clicked() {
    _ui.runLatestOnDesktopCheckBox->setEnabled(!_ui.usePreviousInstallationOnDesktopCheckBox->isChecked());
    _ui.urlOnDesktopLineEdit->setEnabled(!_ui.usePreviousInstallationOnDesktopCheckBox->isChecked() && !_ui.runLatestOnDesktopCheckBox->isChecked());
}

void Nitpick::on_runLatestOnDesktopCheckBox_clicked() {
    _ui.urlOnDesktopLineEdit->setEnabled(!_ui.runLatestOnDesktopCheckBox->isChecked());
}

void Nitpick::on_runFullSuiteOnDesktopCheckBox_clicked() {
    _ui.scriptURLOnDesktopLineEdit->setEnabled(!_ui.runFullSuiteOnDesktopCheckBox->isChecked());
}

void Nitpick::automaticTestRunEvaluationComplete(QString zippedFolderName, int numberOfFailures) {
    _testRunnerDesktop->automaticTestRunEvaluationComplete(zippedFolderName, numberOfFailures);
}

void Nitpick::on_updateTestRailRunResultsPushbutton_clicked() {
    _testCreator->updateTestRailRunResult();
}

// To toggle between show and hide
//   if (uState & ABS_AUTOHIDE) on_showTaskbarButton_clicked();
//   else on_hideTaskbarButton_clicked();
//
void Nitpick::on_hideTaskbarPushbutton_clicked() {
#ifdef Q_OS_WIN
    APPBARDATA abd = { sizeof abd };
    UINT uState = (UINT)SHAppBarMessage(ABM_GETSTATE, &abd);
    LPARAM param = uState & ABS_ALWAYSONTOP;
    abd.lParam = ABS_AUTOHIDE | param;
    SHAppBarMessage(ABM_SETSTATE, &abd);
#endif
}

void Nitpick::on_showTaskbarPushbutton_clicked() {
#ifdef Q_OS_WIN
    APPBARDATA abd = { sizeof abd };
    UINT uState = (UINT)SHAppBarMessage(ABM_GETSTATE, &abd);
    LPARAM param = uState & ABS_ALWAYSONTOP;
    abd.lParam = param;
    SHAppBarMessage(ABM_SETSTATE, &abd);
#endif
}

void Nitpick::on_evaluateTestsPushbutton_clicked() {
    _testCreator->startTestsEvaluation(_ui.gpuVendorComboBox, false, false);
}

void Nitpick::on_closePushbutton_clicked() {
    exit(0);
}

void Nitpick::on_createPythonScriptRadioButton_clicked() {
    _testCreator->setTestRailCreateMode(PYTHON);
}

void Nitpick::on_createXMLScriptRadioButton_clicked() {
    _testCreator->setTestRailCreateMode(XML);
}

void Nitpick::on_createWebPagePushbutton_clicked() {
    _testCreator->createWebPage(_ui.updateAWSCheckBox, _ui.diffImageRadioButton, _ui.ssimImageRadioButton, _ui.awsURLLineEdit, _ui.branchLineEdit->text(), _ui.userLineEdit->text());
}

void Nitpick::about() {
    QMessageBox::information(0, "About", QString("Built ") + __DATE__ + ", " + __TIME__);
}

void Nitpick::content() {
    QDesktopServices::openUrl(QUrl("https://github.com/highfidelity/hifi/blob/master/tools/nitpick/README.md"));
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

void Nitpick::appendLogWindow(const QString& message) {
    _ui.plainTextEdit->appendPlainText(message);
}

// Test on Mobile
void Nitpick::on_setWorkingFolderRunOnMobilePushbutton_clicked() {
    _testRunnerMobile->setWorkingFolderAndEnableControls();
}

void Nitpick::on_connectDevicePushbutton_clicked() {
    _testRunnerMobile->connectDevice();
}

void Nitpick::on_usePreviousInstallationOnMobileCheckBox_clicked() {
    _ui.runLatestOnMobileCheckBox->setEnabled(!_ui.usePreviousInstallationOnMobileCheckBox->isChecked());
    _ui.urlOnMobileLineEdit->setEnabled(!_ui.usePreviousInstallationOnMobileCheckBox->isChecked() && !_ui.runLatestOnMobileCheckBox->isChecked());
}

void Nitpick::on_runLatestOnMobileCheckBox_clicked() {
    _ui.urlOnMobileLineEdit->setEnabled(!_ui.runLatestOnMobileCheckBox->isChecked());
}

void Nitpick::on_runFullSuiteOnMobileCheckBox_clicked() {
    _ui.scriptURLOnMobileLineEdit->setEnabled(!_ui.runFullSuiteOnMobileCheckBox->isChecked());
}

void Nitpick::on_downloadAPKPushbutton_clicked() {
    _testRunnerMobile->downloadAPK();
}

void Nitpick::on_installAPKPushbutton_clicked() {
    _testRunnerMobile->installAPK();
}

void Nitpick::on_runInterfacePushbutton_clicked() {
    _testRunnerMobile->runInterface();
}

void Nitpick::on_pullFolderPushbutton_clicked() {
    _testRunnerMobile->pullFolder();
}
