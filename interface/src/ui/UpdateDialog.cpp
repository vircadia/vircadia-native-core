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
#include <QPushButton>
#include <QLabel>
#include <QFrame>

#include "Application.h"
#include "SharedUtil.h"
#include "UpdateDialog.h"

UpdateDialog::UpdateDialog(QWidget *parent, const QString& releaseNotes, const QString& latestVersion, const QUrl& downloadURL) :
    QWidget(parent, Qt::Widget) {
    
    _latestVersion = latestVersion;
    _downloadUrl = downloadURL;
    
    QUiLoader updateDialogLoader;
    QWidget* updateDialog;
    QFile updateDialogUi("resources/ui/updateDialog.ui");
    updateDialogUi.open(QFile::ReadOnly);
    updateDialog = updateDialogLoader.load(&updateDialogUi, this);
    
    QString updateRequired = QString("You are currently running build %1, the latest build released is %2. \
                                      Please download and install the most recent release to access the latest features and bug fixes.")
                                      .arg(Application::getInstance()->applicationVersion(), latestVersion);
    
    
    setAttribute(Qt::WA_DeleteOnClose);
    
    QPushButton* downloadButton = updateDialog->findChild<QPushButton*>("downloadButton");
    QPushButton* skipButton = updateDialog->findChild<QPushButton*>("skipButton");
    QPushButton* closeButton = updateDialog->findChild<QPushButton*>("closeButton");
    QLabel* updateContent = updateDialog->findChild<QLabel*>("updateContent");
    
    updateContent->setText(updateRequired);
    
    connect(downloadButton, SIGNAL(released()), this, SLOT(handleDownload()));
    connect(skipButton, SIGNAL(released()), this, SLOT(handleSkip()));
    connect(closeButton, SIGNAL(released()), this, SLOT(close()));
    updateDialog->show();
}

void UpdateDialog::handleDownload() {
    QDesktopServices::openUrl(_downloadUrl);
    Application::getInstance()->quit();
}

void UpdateDialog::handleSkip() {
    Application::getInstance()->skipVersion(_latestVersion);
    this->close();
}