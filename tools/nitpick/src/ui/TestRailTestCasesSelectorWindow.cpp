//
//  TestRailTestCasesSelectorWindow.cpp
//
//  Created by Nissim Hadar on 26 Jul 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "TestRailTestCasesSelectorWindow.h"

#include <QtCore/QFileInfo>

#include <cmath>

TestRailTestCasesSelectorWindow::TestRailTestCasesSelectorWindow(QWidget *parent) {
    setupUi(this);

    projectIDLineEdit->setValidator(new QIntValidator(1, 999, this));
}


void TestRailTestCasesSelectorWindow::reset() {
    urlLineEdit->setDisabled(false);
    userLineEdit->setDisabled(false);
    passwordLineEdit->setDisabled(false);
    projectIDLineEdit->setDisabled(false);
    suiteIDLineEdit->setDisabled(false);

    OKButton->setDisabled(true);

    releasesLabel->setDisabled(true);
    releasesComboBox->setDisabled(true);
}

void TestRailTestCasesSelectorWindow::on_acceptButton_clicked() {
    urlLineEdit->setDisabled(true);
    userLineEdit->setDisabled(true);
    passwordLineEdit->setDisabled(true);
    projectIDLineEdit->setDisabled(true);
    suiteIDLineEdit->setDisabled(true);

    OKButton->setDisabled(false);

    releasesLabel->setDisabled(false);
    releasesComboBox->setDisabled(false);
    close();
}

void TestRailTestCasesSelectorWindow::on_OKButton_clicked() {
    userCancelled = false;
    close();
}

void TestRailTestCasesSelectorWindow::on_cancelButton_clicked() {
    userCancelled = true;
    close();
}

bool TestRailTestCasesSelectorWindow::getUserCancelled() {
    return userCancelled;
}

void TestRailTestCasesSelectorWindow::setURL(const QString& user) {
    urlLineEdit->setText(user);
}

QString TestRailTestCasesSelectorWindow::getURL() {
    return urlLineEdit->text();
}

void TestRailTestCasesSelectorWindow::setUser(const QString& user) {
    userLineEdit->setText(user);
}

QString TestRailTestCasesSelectorWindow::getUser() {
    return userLineEdit->text();
}

QString TestRailTestCasesSelectorWindow::getPassword() {
    return passwordLineEdit->text();
}

void TestRailTestCasesSelectorWindow::setProjectID(const int project) {
    projectIDLineEdit->setText(QString::number(project));
}

int TestRailTestCasesSelectorWindow::getProjectID() {
    return projectIDLineEdit->text().toInt();
}

void TestRailTestCasesSelectorWindow::setSuiteID(const int project) {
    suiteIDLineEdit->setText(QString::number(project));
}

int TestRailTestCasesSelectorWindow::getSuiteID() {
    return suiteIDLineEdit->text().toInt();
}

void TestRailTestCasesSelectorWindow::updateReleasesComboBoxData(QStringList data) {
    releasesComboBox->insertItems(0, data);
}

int TestRailTestCasesSelectorWindow::getReleaseID() {
    return releasesComboBox->currentIndex();
}