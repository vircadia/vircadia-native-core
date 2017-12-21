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
#include <QComboBox>

#include <shared/AbstractLoggerInterface.h>

const int REVEAL_BUTTON_WIDTH = 122;
const int DEBUG_CHECKBOX_LEFT = 25;
const int DEBUG_CHECKBOX_WIDTH = 70;
const int INFO_CHECKBOX_WIDTH = 65;
const int CRITICAL_CHECKBOX_WIDTH = 85;

LogDialog::LogDialog(QWidget* parent, AbstractLoggerInterface* logger) : BaseLogDialog(parent) {
    _logger = logger;
    setWindowTitle("Log");

    _revealLogButton = new QPushButton("Reveal log file", this);
    // set object name for css styling
    _revealLogButton->setObjectName("revealLogButton");
    _revealLogButton->show();
    connect(_revealLogButton, SIGNAL(clicked()), SLOT(handleRevealButton()));

    connect(_logger, SIGNAL(logReceived(QString)), this, SLOT(appendLogLine(QString)), Qt::QueuedConnection);

    _leftPad = DEBUG_CHECKBOX_LEFT;

    _debugPrintBox = new QCheckBox("DEBUG", this);
    _debugPrintBox->setGeometry(_leftPad, ELEMENT_MARGIN + ELEMENT_MARGIN + ELEMENT_HEIGHT, DEBUG_CHECKBOX_WIDTH, ELEMENT_HEIGHT);
    /*if (_logger->extraDebugging()) {
    _extraDebuggingBox->setCheckState(Qt::Checked);
    }*/
    _debugPrintBox->show();
    //connect(_extraDebuggingBox, SIGNAL(stateChanged(int)), SLOT(handleExtraDebuggingCheckbox(int)));

    _leftPad += DEBUG_CHECKBOX_WIDTH + BUTTON_MARGIN;

    _infoPrintBox = new QCheckBox("INFO", this);
    _infoPrintBox->setGeometry(_leftPad, ELEMENT_MARGIN + ELEMENT_MARGIN + ELEMENT_HEIGHT, INFO_CHECKBOX_WIDTH, ELEMENT_HEIGHT);
    /*if (_logger->extraDebugging()) {
    _extraDebuggingBox->setCheckState(Qt::Checked);
    }*/
    _infoPrintBox->show();
    //connect(_infoPrintBox, SIGNAL(stateChanged(int)), SLOT(handleExtraDebuggingCheckbox(int)));

    _leftPad += INFO_CHECKBOX_WIDTH + BUTTON_MARGIN;

    _criticalPrintBox = new QCheckBox("CRITICAL", this);
    _criticalPrintBox->setGeometry(_leftPad, ELEMENT_MARGIN + ELEMENT_MARGIN + ELEMENT_HEIGHT, CRITICAL_CHECKBOX_WIDTH, ELEMENT_HEIGHT);
    /*if (_logger->extraDebugging()) {
    _extraDebuggingBox->setCheckState(Qt::Checked);
    }*/
    _criticalPrintBox->show();
    //connect(_criticalPrintBox, SIGNAL(stateChanged(int)), SLOT(handleExtraDebuggingCheckbox(int)));

    _leftPad += CRITICAL_CHECKBOX_WIDTH + BUTTON_MARGIN;

    _extraDebuggingBox = new QCheckBox("Extra debugging", this);
    _extraDebuggingBox->setGeometry(_leftPad, ELEMENT_MARGIN + ELEMENT_MARGIN + ELEMENT_HEIGHT, CHECKBOX_WIDTH, ELEMENT_HEIGHT);
    if (_logger->extraDebugging()) {
        _extraDebuggingBox->setCheckState(Qt::Checked);
    }
    _extraDebuggingBox->show();
    connect(_extraDebuggingBox, SIGNAL(stateChanged(int)), SLOT(handleExtraDebuggingCheckbox(int)));

    _leftPad += CHECKBOX_WIDTH + BUTTON_MARGIN;

    _filterDropdown = new QComboBox(this);
    _filterDropdown->setGeometry(_leftPad, ELEMENT_MARGIN + ELEMENT_MARGIN + ELEMENT_HEIGHT, COMBOBOX_WIDTH, ELEMENT_HEIGHT);
    _filterDropdown->addItem("Select filter...");
    _filterDropdown->addItem("hifi.scriptengine");
    _filterDropdown->addItem("hifi.scriptengine.module");
    _filterDropdown->addItem("hifi.shared");
    _filterDropdown->addItem("hifi.interface.deadlock");
    _filterDropdown->addItem("hifi.scriptengine.script");
    _filterDropdown->addItem("hifi.audioclient");
    _filterDropdown->addItem("hifi.networking");
    _filterDropdown->addItem("hifi.networking.resource");
    _filterDropdown->addItem("hifi.modelformat");
    _filterDropdown->addItem("default");
    _filterDropdown->addItem("qml");
    _filterDropdown->addItem("hifi.ui");
    _filterDropdown->addItem("hifi.avatars");

    
}

void LogDialog::resizeEvent(QResizeEvent* event) {
    BaseLogDialog::resizeEvent(event);
    _revealLogButton->setGeometry(width() - ELEMENT_MARGIN - REVEAL_BUTTON_WIDTH,
        ELEMENT_MARGIN,
        REVEAL_BUTTON_WIDTH,
        ELEMENT_HEIGHT);
    _filterDropdown->setGeometry(width() - ELEMENT_MARGIN - COMBOBOX_WIDTH,
        ELEMENT_MARGIN + ELEMENT_MARGIN + ELEMENT_HEIGHT,
        COMBOBOX_WIDTH,
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
