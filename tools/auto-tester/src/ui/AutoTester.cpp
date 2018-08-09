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

AutoTester::AutoTester(QWidget *parent) : QMainWindow(parent) {
    _ui.setupUi(this);
    _ui.checkBoxInteractiveMode->setChecked(true);
    _ui.progressBar->setVisible(false);

    _signalMapper = new QSignalMapper();

    connect(_ui.actionClose, &QAction::triggered, this, &AutoTester::on_closeButton_clicked);
    connect(_ui.actionAbout, &QAction::triggered, this, &AutoTester::about);

#ifndef Q_OS_WIN
    _ui.hideTaskbarButton->setVisible(false);
    _ui.showTaskbarButton->setVisible(false);
#endif
}

void AutoTester::setup() {
    _test = new Test();
}

void AutoTester::runFromCommandLine(const QString& testFolder, const QString& branch, const QString& user) {
    _isRunningFromCommandline = true;
    _test->startTestsEvaluation(testFolder, branch, user);
}

void AutoTester::on_evaluateTestsButton_clicked() {
    _test->startTestsEvaluation();
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

void AutoTester::on_createTestsOutlineButton_clicked() {
    _test->createTestsOutline();
}

void AutoTester::on_createTestRailTestCasesButton_clicked() {
    _test->createTestRailTestCases();
}

void AutoTester::on_createTestRailRunButton_clicked() {
    _test->createTestRailRun();
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

void AutoTester::downloadImage(const QUrl& url) {
    _downloaders.emplace_back(new Downloader(url, this));
    connect(_downloaders[_index], SIGNAL (downloaded()), _signalMapper, SLOT (map()));

    _signalMapper->setMapping(_downloaders[_index], _index);

    ++_index;
}

void AutoTester::downloadImages(const QStringList& URLs, const QString& directoryName, const QStringList& filenames) {
    _directoryName = directoryName;
    _filenames = filenames;

    _numberOfImagesToDownload = URLs.size();
    _numberOfImagesDownloaded = 0;
    _index = 0;

    _ui.progressBar->setMinimum(0);
    _ui.progressBar->setMaximum(_numberOfImagesToDownload - 1);
    _ui.progressBar->setValue(0);
    _ui.progressBar->setVisible(true);

    _downloaders.clear();
    for (int i = 0; i < _numberOfImagesToDownload; ++i) {
        QUrl imageURL(URLs[i]);
        downloadImage(imageURL);
    }

    connect(_signalMapper, SIGNAL (mapped(int)), this, SLOT (saveImage(int)));
}

void AutoTester::saveImage(int index) {
    try {
        QFile file(_directoryName + "/" + _filenames[index]);
        file.open(QIODevice::WriteOnly);
        file.write(_downloaders[index]->downloadedData());
        file.close();
    } catch (...) {
        QMessageBox::information(0, "Test Aborted", "Failed to save image: " + _filenames[index]);
        _ui.progressBar->setVisible(false);
        return;
    }

    ++_numberOfImagesDownloaded;

    if (_numberOfImagesDownloaded == _numberOfImagesToDownload) {
        disconnect(_signalMapper, SIGNAL (mapped(int)), this, SLOT (saveImage(int)));
        _test->finishTestsEvaluation(_isRunningFromCommandline, _ui.checkBoxInteractiveMode->isChecked(), _ui.progressBar);
    } else {
        _ui.progressBar->setValue(_numberOfImagesDownloaded);
    }
}

void AutoTester::about() {
    QMessageBox::information(0, "About", QString("Built ") + __DATE__ + " : " + __TIME__);
}

void AutoTester::setUserText(const QString& user) {
    _ui.userTextEdit->setText(user);
}

QString AutoTester::getSelectedUser()
{
    return _ui.userTextEdit->toPlainText();
}

void AutoTester::setBranchText(const QString& branch) {
    _ui.branchTextEdit->setText(branch);
}

QString AutoTester::getSelectedBranch() {
    return _ui.branchTextEdit->toPlainText();
}
