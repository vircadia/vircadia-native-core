//
//  UpdateDialog.cpp
//  interface/src/ui
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <gpu/GPUConfig.h>

#include <QtGui>
#include "ui_updateDialog.h"

#include "Application.h"
#include "UpdateDialog.h"


UpdateDialog::UpdateDialog(QWidget *parent, const QString& releaseNotes, const QString& latestVersion, const QUrl& downloadURL) :
    QDialog(parent),
    _latestVersion(latestVersion),
    _downloadUrl(downloadURL)
{
    Ui::Dialog dialogUI;
    dialogUI.setupUi(this);
    
    QString updateRequired = QString("You are currently running build %1, the latest build released is %2."
                                     "\n\nPlease download and install the most recent release to access the latest features and bug fixes.")
                                      .arg(Application::getInstance()->applicationVersion(), latestVersion);
    
    setAttribute(Qt::WA_DeleteOnClose);
    
    QPushButton* downloadButton = findChild<QPushButton*>("downloadButton");
    QPushButton* skipButton = findChild<QPushButton*>("skipButton");
    QPushButton* closeButton = findChild<QPushButton*>("closeButton");
    QLabel* updateContent = findChild<QLabel*>("updateContent");
    
    updateContent->setText(updateRequired);
    
    connect(downloadButton, SIGNAL(released()), this, SLOT(handleDownload()));
    connect(skipButton, SIGNAL(released()), this, SLOT(handleSkip()));
    connect(closeButton, SIGNAL(released()), this, SLOT(close()));
    
    QMetaObject::invokeMethod(this, "show", Qt::QueuedConnection);
}

void UpdateDialog::handleDownload() {
    QDesktopServices::openUrl(_downloadUrl);
    Application::getInstance()->quit();
}

void UpdateDialog::handleSkip() {
    Application::getInstance()->skipVersion(_latestVersion);
    this->close();
}
