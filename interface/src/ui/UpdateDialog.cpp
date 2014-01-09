//
//  UpdateDialog.cpp
//  interface
//
//  Created by Leonardo Murillo <leo@highfidelity.io> on 1/8/14.
//  Copyright (c) 2013, 2014 High Fidelity, Inc. All rights reserved.
//

#include <QDesktopWidget>
#include <QTextBlock>
#include <QtGui>

#include "SharedUtil.h"
#include "UpdateDialog.h"

const int buttonWidth = 120;
const int buttonHeight = 40;
const int dialogWidth = 500;
const int dialogHeigth = 300;

const QString dialogTitle = "Update Required";

UpdateDialog::UpdateDialog(QWidget *parent, QString releaseNotes, QUrl *downloadURL, QString latestVersion, QString currentVersion) : QDialog(parent, Qt::Dialog) {
    
    const QString updateRequired = QString("You are currently running build %1, the latest build released is %2.\n \
                                            Please download and install the most recent release to access the latest \
                                            features and bug fixes.").arg(currentVersion, latestVersion);
    
    setWindowTitle(dialogTitle);
    setModal(true);
    QFile styleSheet("resources/styles/update_dialog.qss");
    if (styleSheet.open(QIODevice::ReadOnly)) {
        setStyleSheet(styleSheet.readAll());
    }
    _releaseNotes = new QLabel(this);
    _releaseNotes->setText(releaseNotes);
    
    _updateRequired = new QLabel(this);
    _updateRequired->setText(updateRequired);
    
    _downloadButton = new QPushButton("Download", this);
    _downloadButton->setObjectName("downloadButton");
    _skipButton = new QPushButton("Skip Version", this);
    _skipButton->setObjectName("skipButton");
    _closeButton = new QPushButton("Close", this);
    _closeButton->setObjectName("closeButton");
    
    _titleBackground = new QFrame();
    
    connect(_downloadButton, SIGNAL(released()), this, SLOT(handleDownload()));
    connect(_skipButton, SIGNAL(released()), this, SLOT(handleDownload()));
    connect(_closeButton, SIGNAL(released()), this, SLOT(handleClose()));
}

void UpdateDialog::handleDownload() {
    qDebug("download clicked");
}

void UpdateDialog::handleSkip() {
    qDebug("skip clicked");
}

void UpdateDialog::handleClose() {
    qDebug("close clicked");
}