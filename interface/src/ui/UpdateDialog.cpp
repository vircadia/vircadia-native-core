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

const QString dialogTitle = "Update Required";
const QString updateRequired = QString("You are currently running build %1, the latest build released is %2.\n \
                                       Please download and install the most recent release to access the latest \
                                       features and bug fixes.").arg("1", "2");

UpdateDialog::UpdateDialog(QWidget *parent, QString releaseNotes, QString downloadURL) : QDialog(parent, Qt::Dialog) {
    setWindowTitle(dialogTitle);
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
    
    
}