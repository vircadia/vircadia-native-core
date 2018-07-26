//
//  TestRailSelectorWindow.cpp
//
//  Created by Nissim Hadar on 26 Jul 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "TestRailSelectorWindow.h"

#include <QtCore/QFileInfo>

#include <cmath>

TestRailSelectorWindow::TestRailSelectorWindow(QWidget *parent) {
    setupUi(this);
}

void TestRailSelectorWindow::on_OKButton_clicked() {
    userCancelled = false;
    close();
}

void TestRailSelectorWindow::on_cancelButton_clicked() {
    userCancelled = true;
    close();
}

bool TestRailSelectorWindow::getUserCancelled() {
    return userCancelled;
}

void TestRailSelectorWindow::setURL(const QString& user) {
    URLTextEdit->setText(user);
}

QString TestRailSelectorWindow::getURL() {
    return URLTextEdit->toPlainText();
}

void TestRailSelectorWindow::setUser(const QString& user) {
    UserTextEdit->setText(user);
}

QString TestRailSelectorWindow::getUser() {
    return UserTextEdit->toPlainText();
}

QString TestRailSelectorWindow::getPassword() {
    return passwordLineEdit->text();
}