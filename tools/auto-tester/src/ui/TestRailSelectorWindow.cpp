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

    projectIDLineEdit->setValidator(new QIntValidator(1, 999, this));
}


void TestRailSelectorWindow::reset() {
    urlLineEdit->setDisabled(false);
    userLineEdit->setDisabled(false);
    passwordLineEdit->setDisabled(false);
    projectIDLineEdit->setDisabled(false);

    OKButton->setDisabled(true);
    milestoneComboBox->setDisabled(true);
}

void TestRailSelectorWindow::on_acceptButton_clicked() {
    urlLineEdit->setDisabled(true);
    userLineEdit->setDisabled(true);
    passwordLineEdit->setDisabled(true);
    projectIDLineEdit->setDisabled(true);

    OKButton->setDisabled(false);
    milestoneComboBox->setDisabled(false);
    close();
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
    urlLineEdit->setText(user);
}

QString TestRailSelectorWindow::getURL() {
    return urlLineEdit->text();
}

void TestRailSelectorWindow::setUser(const QString& user) {
    userLineEdit->setText(user);
}

QString TestRailSelectorWindow::getUser() {
    return userLineEdit->text();
}

QString TestRailSelectorWindow::getPassword() {
    return passwordLineEdit->text();
}

void TestRailSelectorWindow::setProjectID(const int project) {
    projectIDLineEdit->setText(QString::number(project));
}

int TestRailSelectorWindow::getProjectID() {
    return projectIDLineEdit->text().toInt();
}

void TestRailSelectorWindow::setSuiteID(const int project) {
    suiteIDLineEdit->setText(QString::number(project));
}

int TestRailSelectorWindow::getSuiteID() {
    return suiteIDLineEdit->text().toInt();
}

void TestRailSelectorWindow::updateMilestoneComboBoxData(QStringList data) {
    milestoneComboBox->insertItems(0, data);
}

int TestRailSelectorWindow::getMilestoneID() {
    return milestoneComboBox->currentIndex();
}