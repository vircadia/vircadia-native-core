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
    
    Application* application = Application::getInstance();
    
    const QString updateRequired = QString("You are currently running build %1, the latest build released is %2.\n \
                                            Please download and install the most recent release to access the latest \
                                            features and bug fixes.").arg(application->applicationVersion(), *application->_latestVersion);
    
    int leftPosition = leftStartingPosition;
    setWindowTitle(dialogTitle);
    //setWindowFlags(Qt::WindowTitleHint);
    setModal(true);
    resize(dialogWidth, dialogHeigth);
    QFile styleSheet("resources/styles/update_dialog.qss");
    if (styleSheet.open(QIODevice::ReadOnly)) {
        setStyleSheet(styleSheet.readAll());
    }
    _releaseNotes = new QLabel(this);
    _releaseNotes->setText(releaseNotes);
    _releaseNotes->setObjectName("releaseNotes");
    
    _updateRequired = new QLabel(this);
    _updateRequired->setText(updateRequired);
    _updateRequired->setObjectName("updateRequired");
    
    _downloadButton = new QPushButton("Download", this);
    _downloadButton->setObjectName("downloadButton");
    _downloadButton->setGeometry(leftPosition, buttonMargin, buttonWidth, buttonHeight);
    leftPosition += buttonWidth;
    
    _skipButton = new QPushButton("Skip Version", this);
    _skipButton->setObjectName("skipButton");
    _skipButton->setGeometry(leftPosition, buttonMargin, buttonWidth, buttonHeight);
    leftPosition += buttonWidth;
    
    _closeButton = new QPushButton("Close", this);
    _closeButton->setObjectName("closeButton");
    _closeButton->setGeometry(leftPosition, buttonMargin, buttonWidth, buttonHeight);
    
    _titleBackground = new QFrame();
    
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
    close();
}

void UpdateDialog::handleClose() {
    close();
}