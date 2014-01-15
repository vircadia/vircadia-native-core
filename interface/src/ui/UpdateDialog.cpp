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

UpdateDialog::UpdateDialog(QWidget *parent, QString releaseNotes) : QDialog(parent, Qt::Dialog) {
    Application* application = Application::getInstance();
    
    QUiLoader updateDialogLoader;
    
    QFile updateDialogUi("resources/ui/updateDialog.ui");
    updateDialogUi.open(QFile::ReadOnly);
    dialogWidget = updateDialogLoader.load(&updateDialogUi, parent);
    updateDialogUi.close();
    
    const QString updateRequired = QString("You are currently running build %1, the latest build released is %2. \
                                           Please download and install the most recent release to access the latest features and bug fixes.")
                                           .arg(application->applicationVersion(), *application->_latestVersion);
    
    
    setAttribute(Qt::WA_DeleteOnClose);
    
    QPushButton *_downloadButton = dialogWidget->findChild<QPushButton *>("downloadButton");
    QPushButton *_skipButton = dialogWidget->findChild<QPushButton *>("skipButton");
    QPushButton *_closeButton = dialogWidget->findChild<QPushButton *>("closeButton");
    QLabel *_updateContent = dialogWidget->findChild<QLabel *>("updateContent");
    
    _updateContent->setText(updateRequired);
    
    connect(_downloadButton, SIGNAL(released()), this, SLOT(handleDownload()));
    connect(_skipButton, SIGNAL(released()), this, SLOT(handleSkip()));
    connect(_closeButton, SIGNAL(released()), this, SLOT(handleClose()));
    dialogWidget->show();
}

UpdateDialog::~UpdateDialog() {
    deleteLater();
}

void UpdateDialog::toggleUpdateDialog() {
    if (this->dialogWidget->isVisible()) {
        this->dialogWidget->hide();
    } else {
        this->dialogWidget->show();
    }
}

void UpdateDialog::handleDownload() {
    Application* application = Application::getInstance();
    QDesktopServices::openUrl(*application->_downloadURL);
    application->quit();
}

void UpdateDialog::handleSkip() {
    Application* application = Application::getInstance();
    application->skipVersion();
    this->toggleUpdateDialog();
}

void UpdateDialog::handleClose() {
    this->toggleUpdateDialog();
}