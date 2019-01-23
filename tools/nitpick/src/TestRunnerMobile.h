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

#include <QLabel>
#include <QObject>
#include <QPushButton>

#include "TestRunner.h"

class TestRunnerMobile : public QObject, public TestRunner {
    Q_OBJECT
public:
    explicit TestRunnerMobile(QLabel* workingFolderLabel, QPushButton *readDeviceButton, QObject* parent = 0);
    ~TestRunnerMobile();

    void setWorkingFolderAndEnableControls();
    void readDevice();

private:
    QLabel* _workingFolderLabel;
    QString _workingFolder;
    QPushButton* _readDeviceButton;
};
#endif
