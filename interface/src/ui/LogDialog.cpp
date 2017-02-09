//
//  LogDialog.cpp
//  interface/src/ui
//
//  Created by Stojce Slavkovski on 12/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LogDialog.h"

#include <QCheckBox>
#include <QPushButton>

#include <shared/AbstractLoggerInterface.h>

const int REVEAL_BUTTON_WIDTH = 122;

LogDialog::LogDialog(QWidget* parent, AbstractLoggerInterface* logger) : BaseLogDialog(parent) {
    _logger = logger;
    setWindowTitle("Log");

    _extraDebuggingBox = new QCheckBox("Extra debugging", this);
    _extraDebuggingBox->setGeometry(_leftPad, ELEMENT_MARGIN, CHECKBOX_WIDTH, ELEMENT_HEIGHT);
    if (_logger->extraDebugging()) {
        _extraDebuggingBox->setCheckState(Qt::Checked);
    }
    _extraDebuggingBox->show();
    connect(_extraDebuggingBox, SIGNAL(stateChanged(int)), SLOT(handleExtraDebuggingCheckbox(int)));

    _revealLogButton = new QPushButton("Reveal log file", this);
    // set object name for css styling
    _revealLogButton->setObjectName("revealLogButton");
    _revealLogButton->show();
    connect(_revealLogButton, SIGNAL(clicked()), SLOT(handleRevealButton()));

    connect(_logger, SIGNAL(logReceived(QString)), this, SLOT(appendLogLine(QString)), Qt::QueuedConnection);
}

void LogDialog::resizeEvent(QResizeEvent* event) {
    BaseLogDialog::resizeEvent(event);
    _revealLogButton->setGeometry(width() - ELEMENT_MARGIN - REVEAL_BUTTON_WIDTH,
                                  ELEMENT_MARGIN,
                                  REVEAL_BUTTON_WIDTH,
                                  ELEMENT_HEIGHT);
}

void LogDialog::handleRevealButton() {
    _logger->locateLog();
}

void LogDialog::handleExtraDebuggingCheckbox(int state) {
    _logger->setExtraDebugging(state != 0);
}

QString LogDialog::getCurrentLog() {
    return _logger->getLogData();
}
