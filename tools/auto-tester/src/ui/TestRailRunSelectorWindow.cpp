//
//  TestRailRunSelectorWindow.cpp
//
//  Created by Nissim Hadar on 31 Jul 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "TestRailRunSelectorWindow.h"

#include <QtCore/QFileInfo>

#include <cmath>

TestRailRunSelectorWindow::TestRailRunSelectorWindow(QWidget *parent) {
    setupUi(this);

    projectIDLineEdit->setValidator(new QIntValidator(1, 999, this));
}


void TestRailRunSelectorWindow::reset() {
    urlLineEdit->setDisabled(false);
    userLineEdit->setDisabled(false);
    passwordLineEdit->setDisabled(false);
    projectIDLineEdit->setDisabled(false);

    OKButton->setDisabled(true);
    sectionsComboBox->setDisabled(true);
}

void TestRailRunSelectorWindow::on_acceptButton_clicked() {
    urlLineEdit->setDisabled(true);
    userLineEdit->setDisabled(true);
    passwordLineEdit->setDisabled(true);
    projectIDLineEdit->setDisabled(true);

    OKButton->setDisabled(false);
    sectionsComboBox->setDisabled(false);
    close();
}

void TestRailRunSelectorWindow::on_OKButton_clicked() {
    userCancelled = false;
    close();
}

void TestRailRunSelectorWindow::on_cancelButton_clicked() {
    userCancelled = true;
    close();
}

bool TestRailRunSelectorWindow::getUserCancelled() {
    return userCancelled;
}

void TestRailRunSelectorWindow::setURL(const QString& user) {
    urlLineEdit->setText(user);
}

QString TestRailRunSelectorWindow::getURL() {
    return urlLineEdit->text();
}

void TestRailRunSelectorWindow::setUser(const QString& user) {
    userLineEdit->setText(user);
}

QString TestRailRunSelectorWindow::getUser() {
    return userLineEdit->text();
}

QString TestRailRunSelectorWindow::getPassword() {
    return passwordLineEdit->text();
}

void TestRailRunSelectorWindow::setProjectID(const int project) {
    projectIDLineEdit->setText(QString::number(project));
}

int TestRailRunSelectorWindow::getProjectID() {
    return projectIDLineEdit->text().toInt();
}

void TestRailRunSelectorWindow::setSuiteID(const int project) {
    suiteIDLineEdit->setText(QString::number(project));
}

int TestRailRunSelectorWindow::getSuiteID() {
    return suiteIDLineEdit->text().toInt();
}

void TestRailRunSelectorWindow::updateSectionsComboBoxData(QStringList data) {
    sectionsComboBox->insertItems(0, data);
}

int TestRailRunSelectorWindow::getSectionID() {
    return 0;
    sectionsComboBox->currentIndex();
}