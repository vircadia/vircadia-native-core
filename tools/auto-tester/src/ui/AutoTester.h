//
//  AutoTester.h
//  zone/ambientLightInheritence
//
//  Created by Nissim Hadar on 2 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_AutoTester_h
#define hifi_AutoTester_h

#include <QtWidgets/QMainWindow>
#include "ui_AutoTester.h"
#include "../Test.h"

class AutoTester : public QMainWindow {
    Q_OBJECT

public:
    AutoTester(QWidget *parent = Q_NULLPTR);

private slots:
void on_evaluateTestsButton_clicked();
void on_evaluateTestsRecursivelyButton_clicked();
void on_createRecursiveScriptButton_clicked();
    void on_createTestButton_clicked();
    void on_closeButton_clicked();

private:
    Ui::AutoTesterClass ui;

    Test test;
};

#endif // hifi_AutoTester_h