//
//  TestRailResultsSelectorWindow.cpp
//
//  Created by Nissim Hadar on 2 Aug 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "TestRailResultsSelectorWindow.h"

#include <QtCore/QFileInfo>

#include <cmath>

TestRailResultsSelectorWindow::TestRailResultsSelectorWindow(QWidget *parent) {
    setupUi(this);

    projectIDLineEdit->setValidator(new QIntValidator(1, 999, this));
}


void TestRailResultsSelectorWindow::reset() {
    urlLineEdit->setDisabled(false);
    userLineEdit->setDisabled(false);
    passwordLineEdit->setDisabled(false);
    projectIDLineEdit->setDisabled(false);
    suiteIDLineEdit->setDisabled(false);

    OKButton->setDisabled(true);

    runsLabel->setDisabled(true);
    runsComboBox->setDisabled(true);
}

void TestRailResultsSelectorWindow::on_acceptButton_clicked() {
    urlLineEdit->setDisabled(true);
    userLineEdit->setDisabled(true);
    passwordLineEdit->setDisabled(true);
    projectIDLineEdit->setDisabled(true);
    suiteIDLineEdit->setDisabled(true);

    OKButton->setDisabled(false);

    runsLabel->setDisabled(false);
    runsComboBox->setDisabled(false);
    close();
}

void TestRailResultsSelectorWindow::on_OKButton_clicked() {
    userCancelled = false;
    close();
}

void TestRailResultsSelectorWindow::on_cancelButton_clicked() {
    userCancelled = true;
    close();
}

bool TestRailResultsSelectorWindow::getUserCancelled() {
    return userCancelled;
}

void TestRailResultsSelectorWindow::setURL(const QString& user) {
    urlLineEdit->setText(user);
}

QString TestRailResultsSelectorWindow::getURL() {
    return urlLineEdit->text();
}

void TestRailResultsSelectorWindow::setUser(const QString& user) {
    userLineEdit->setText(user);
}

QString TestRailResultsSelectorWindow::getUser() {
    return userLineEdit->text();
}

QString TestRailResultsSelectorWindow::getPassword() {
    return passwordLineEdit->text();
}

void TestRailResultsSelectorWindow::setProjectID(const int project) {
    projectIDLineEdit->setText(QString::number(project));
}

int TestRailResultsSelectorWindow::getProjectID() {
    return projectIDLineEdit->text().toInt();
}

void TestRailResultsSelectorWindow::setSuiteID(const int project) {
    suiteIDLineEdit->setText(QString::number(project));
}

int TestRailResultsSelectorWindow::getSuiteID() {
    return suiteIDLineEdit->text().toInt();
}

void TestRailResultsSelectorWindow::updateRunsComboBoxData(QStringList data) {
    runsComboBox->insertItems(0, data);
}

int TestRailResultsSelectorWindow::getRunID() {
    return runsComboBox->currentIndex();
}