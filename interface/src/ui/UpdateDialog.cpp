//
//  UpdateDialog.cpp
//  interface
//
//  Created by Leonardo Murillo <leo@highfidelity.io> on 1/8/14.
//  Copyright (c) 2013, 2014 High Fidelity, Inc. All rights reserved.
//

#include <QApplication>
#include <QDesktopWidget>
#include <QTextBlock>
#include <QtGui>
#include <QtUiTools>

#include "Application.h"
#include "SharedUtil.h"
#include "UpdateDialog.h"

const int buttonWidth = 125;
const int buttonHeight = 40;
const int buttonMargin = 100;
const int leftStartingPosition = 275;
const int dialogWidth = 750;
const int dialogHeigth = 300;

const QString dialogTitle = "Update Required";

UpdateDialog::UpdateDialog(QWidget *parent, QString releaseNotes) : QDialog(parent, Qt::Dialog) {
    
    QUiLoader updateDialogLoader;
    
    QFile updateDialogUi("resources/ui/updateDialog.ui");
    updateDialogUi.open(QFile::ReadOnly);
    QWidget *updateDialog = updateDialogLoader.load(&updateDialogUi, this);
    updateDialogUi.close();
    
    updateDialog->show();
    
    Application* application = Application::getInstance();
    
    const QString updateRequired = QString("You are currently running build %1, the latest build released is %2. Please download and install the most recent release to access the latest features and bug fixes.").arg(application->applicationVersion(), *application->_latestVersion);
    
    
    QPushButton *_downloadButton = updateDialog->findChild<QPushButton*>("downloadButton");
    QPushButton *_skipButton = updateDialog->findChild<QPushButton*>("skipButton");
    QPushButton *_closeButton = updateDialog->findChild<QPushButton*>("closeButton");
    QLabel *_updateContent = updateDialog->findChild<QLabel*>("updateContent");
    
    _updateContent->setText(updateRequired);
    
    connect(_downloadButton, SIGNAL(released()), this, SLOT(handleDownload()));
    connect(_skipButton, SIGNAL(released()), this, SLOT(handleSkip()));
    connect(_closeButton, SIGNAL(released()), this, SLOT(handleClose()));
}

void UpdateDialog::handleDownload() {
    Application* application = Application::getInstance();
    QDesktopServices::openUrl(*application->_downloadURL);
    application->quit();
}

void UpdateDialog::handleSkip() {
    Application* application = Application::getInstance();
    application->skipVersion();
    this->QDialog::close();
}

void UpdateDialog::handleClose() {
    qDebug("###### HANDLECLOSE\n");
    this->QDialog::close();
}