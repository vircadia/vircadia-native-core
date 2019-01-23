//
//  TestRunnerMobile.cpp
//
//  Created by Nissim Hadar on 22 Jan 2019.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "TestRunnerMobile.h"

#include <QThread>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>

#include "Nitpick.h"
extern Nitpick* nitpick;

TestRunnerMobile::TestRunnerMobile(QLabel* workingFolderLabel, QPushButton *readDeviceButton, QObject* parent) : QObject(parent) {
    _workingFolderLabel = workingFolderLabel;
    _readDeviceButton = readDeviceButton;
}

TestRunnerMobile::~TestRunnerMobile() {
}

void TestRunnerMobile::setWorkingFolderAndEnableControls() {
    setWorkingFolder(_workingFolderLabel);

    _readDeviceButton->setEnabled(true);
}

void TestRunnerMobile::readDevice() {
}
