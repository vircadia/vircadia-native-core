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
#include <QPlainTextEdit>

#include <shared/AbstractLoggerInterface.h>

const int REVEAL_BUTTON_WIDTH = 122;
const int SHOW_ALL_BUTTON_WIDTH = 80;
const int DEBUG_CHECKBOX_LEFT = 25;
const int DEBUG_CHECKBOX_WIDTH = 70;
const int INFO_CHECKBOX_WIDTH = 65;
const int CRITICAL_CHECKBOX_WIDTH = 85;
const QString DEBUG = "[DEBUG]";
const QString INFO = "[INFO]";

LogDialog::LogDialog(QWidget* parent, AbstractLoggerInterface* logger) : BaseLogDialog(parent) {
    _logger = logger;
    setWindowTitle("Log");

    _revealLogButton = new QPushButton("Reveal log file", this);
    // set object name for css styling
    _revealLogButton->setObjectName("revealLogButton");
    _revealLogButton->show();
    connect(_revealLogButton, SIGNAL(clicked()), SLOT(handleRevealButton()));

    _showAllButton = new QPushButton("Show All", this);
    // set object name for css styling
    _showAllButton->setObjectName("showAllButton");
    _showAllButton->show();
    connect(_showAllButton, SIGNAL(clicked()), SLOT(handleShowAllButton()));

    connect(_logger, SIGNAL(logReceived(QString)), this, SLOT(appendLogLine(QString)), Qt::QueuedConnection);

    _leftPad = DEBUG_CHECKBOX_LEFT;

    _debugPrintBox = new QCheckBox("DEBUG", this);
    _debugPrintBox->setGeometry(_leftPad, ELEMENT_MARGIN + ELEMENT_MARGIN + ELEMENT_HEIGHT, DEBUG_CHECKBOX_WIDTH, ELEMENT_HEIGHT);
    if (_logger->debugPrint()) {
        _debugPrintBox->setCheckState(Qt::Checked);
    }
    _debugPrintBox->show();
    connect(_debugPrintBox, SIGNAL(stateChanged(int)), SLOT(handleDebugPrintBox(int)));

    _leftPad += DEBUG_CHECKBOX_WIDTH + BUTTON_MARGIN;

    _infoPrintBox = new QCheckBox("INFO", this);
    _infoPrintBox->setGeometry(_leftPad, ELEMENT_MARGIN + ELEMENT_MARGIN + ELEMENT_HEIGHT, INFO_CHECKBOX_WIDTH, ELEMENT_HEIGHT);
    if (_logger->infoPrint()) {
        _infoPrintBox->setCheckState(Qt::Checked);
    }
    _infoPrintBox->show();
    connect(_infoPrintBox, SIGNAL(stateChanged(int)), SLOT(handleInfoPrintBox(int)));

    _leftPad += INFO_CHECKBOX_WIDTH + BUTTON_MARGIN;

    _criticalPrintBox = new QCheckBox("CRITICAL", this);
    _criticalPrintBox->setGeometry(_leftPad, ELEMENT_MARGIN + ELEMENT_MARGIN + ELEMENT_HEIGHT, CRITICAL_CHECKBOX_WIDTH, ELEMENT_HEIGHT);
    if (_logger->criticalPrint()) {
        _criticalPrintBox->setCheckState(Qt::Checked);
    }
    _criticalPrintBox->show();
    connect(_criticalPrintBox, SIGNAL(stateChanged(int)), SLOT(handleCriticalPrintBox(int)));

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
    _filterDropdown->addItem("No secondary filter...");
    _filterDropdown->addItem("default");
    _filterDropdown->addItem("hifi.audio");
    _filterDropdown->addItem("hifi.audioclient");
    _filterDropdown->addItem("hifi.animation");
    _filterDropdown->addItem("hifi.avatars");
    _filterDropdown->addItem("hifi.commerce");
    _filterDropdown->addItem("hifi.controllers");
    _filterDropdown->addItem("hifi.entities");
    _filterDropdown->addItem("hifi.gl");
    _filterDropdown->addItem("hifi.gpu.gl");
    _filterDropdown->addItem("hifi.interface");
    _filterDropdown->addItem("hifi.interface.deadlock");
    _filterDropdown->addItem("hifi.modelformat");
    _filterDropdown->addItem("hifi.networking");
    _filterDropdown->addItem("hifi.networking.resource");
    _filterDropdown->addItem("hifi.plugins");
    _filterDropdown->addItem("hifi.render");
    _filterDropdown->addItem("hifi.scriptengine");
    _filterDropdown->addItem("hifi.scriptengine.module");
    _filterDropdown->addItem("hifi.scriptengine.script");
    _filterDropdown->addItem("hifi.shared");
    _filterDropdown->addItem("hifi.ui");
    _filterDropdown->addItem("qml");

    connect(_filterDropdown, SIGNAL(currentIndexChanged(int)), this, SLOT(handleFilterDropdownChanged(int)));
}

void LogDialog::resizeEvent(QResizeEvent* event) {
    BaseLogDialog::resizeEvent(event);
    _revealLogButton->setGeometry(width() - ELEMENT_MARGIN - REVEAL_BUTTON_WIDTH,
        ELEMENT_MARGIN,
        REVEAL_BUTTON_WIDTH,
        ELEMENT_HEIGHT);
    _showAllButton->setGeometry(width() - ELEMENT_MARGIN - REVEAL_BUTTON_WIDTH - ELEMENT_MARGIN - SHOW_ALL_BUTTON_WIDTH,
        ELEMENT_MARGIN,
        SHOW_ALL_BUTTON_WIDTH,
        ELEMENT_HEIGHT);
    _filterDropdown->setGeometry(width() - ELEMENT_MARGIN - COMBOBOX_WIDTH,
        ELEMENT_MARGIN + ELEMENT_MARGIN + ELEMENT_HEIGHT,
        COMBOBOX_WIDTH,
        ELEMENT_HEIGHT);
}

void LogDialog::handleRevealButton() {
    _logger->locateLog();
}

void LogDialog::handleShowAllButton() {
    _logTextBox->clear();
    log = getCurrentLog();
    _logTextBox->appendPlainText(log);
}

void LogDialog::handleExtraDebuggingCheckbox(int state) {
    _logger->setExtraDebugging(state != 0);
}

void LogDialog::handleDebugPrintBox(int state) {
    _logger->setDebugPrint(state != 0);
    printLogFile();
}

void LogDialog::handleInfoPrintBox(int state) {
    _logger->setInfoPrint(state != 0);
    printLogFile();
}

void LogDialog::handleCriticalPrintBox(int state) {
    _logger->setCriticalPrint(state != 0);
    printLogFile();
}

void LogDialog::handleFilterDropdownChanged(int selection) {
    if (selection != 0) {
        _filterSelection = "[" + _filterDropdown->currentText() + "]";
    } else {
        _filterSelection = "";
    }
    printLogFile();
}

QString LogDialog::getCurrentLog() {
    return _logger->getLogData();
}

void LogDialog::appendLogLine(QString logLine) {
    if (logLine.contains(_searchTerm, Qt::CaseInsensitive) &&
        logLine.contains(_filterSelection, Qt::CaseSensitive)) {
        int indexToBold = _logTextBox->document()->characterCount();
        _highlighter->setBold(indexToBold);

        if (logLine.contains(DEBUG, Qt::CaseSensitive)) {
            if (_logger->debugPrint()) {
                _logTextBox->appendPlainText(logLine.trimmed());
            }
        }
        else if (logLine.contains(INFO, Qt::CaseSensitive)) {
            if (_logger->infoPrint()) {
                _logTextBox->appendPlainText(logLine.trimmed());
            }
        }
        else {
            if (_logger->criticalPrint()) {
                _logTextBox->appendPlainText(logLine.trimmed());
            }
        }
    }
}

void LogDialog::printLogFile() {
    _logTextBox->clear();
    log = getCurrentLog();
    QStringList logList = log.split('\n');
    for (const auto& message : logList) {
        appendLogLine(message);
    }
}
