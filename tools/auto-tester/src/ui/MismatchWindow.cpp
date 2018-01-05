//
//  MismatchWindow.cpp
//
//  Created by Nissim Hadar on 9 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "MismatchWindow.h"

#include <QtCore/QFileInfo>

MismatchWindow::MismatchWindow(QWidget *parent) : QDialog(parent) {
    setupUi(this);

    expectedImage->setScaledContents(true);
    resultImage->setScaledContents(true);
}

void MismatchWindow::setTestFailure(TestFailure testFailure) {
    errorLabel->setText("Similarity: " + QString::number(testFailure._error));

    imagePath->setText("Path to test: " + testFailure._pathname);

    expectedFilename->setText(testFailure._expectedImageFilename);
    expectedImage->setPixmap(QPixmap(testFailure._pathname + testFailure._expectedImageFilename));

    resultFilename->setText(testFailure._actualImageFilename);
    resultImage->setPixmap(QPixmap(testFailure._pathname + testFailure._actualImageFilename));
}

void MismatchWindow::on_passTestButton_clicked() {
    _userResponse = USER_RESPONSE_PASS;
    close();
}

void MismatchWindow::on_failTestButton_clicked() {
    _userResponse = USE_RESPONSE_FAIL;
    close();
}

void MismatchWindow::on_abortTestsButton_clicked() {
    _userResponse = USER_RESPONSE_ABORT;
    close();
}
