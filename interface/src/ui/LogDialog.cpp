//
//  LogDialog.cpp
//  interface
//
//  Created by Stojce Slavkovski on 12/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QDesktopWidget>
#include <QTextBlock>
#include <QtGui>

#include "SharedUtil.h"
#include "ui/LogDialog.h"
#include "LogDisplay.h"

#define INITIAL_WIDTH_RATIO 0.3
#define INITIAL_HEIGHT_RATIO 0.6

int cursorMeta = qRegisterMetaType<QTextCursor>("QTextCursor");
int blockMeta = qRegisterMetaType<QTextBlock>("QTextBlock");

LogDialog::LogDialog(QWidget* parent) :
    QDialog(parent, Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint) {

        setWindowTitle("Log");

        _logTextBox = new QPlainTextEdit(this);
        _logTextBox->setReadOnly(true);
        _logTextBox->show();

        switchToResourcesParentIfRequired();
        QFile styleSheet("resources/styles/log_dialog.qss");

        if (styleSheet.open(QIODevice::ReadOnly)) {
            setStyleSheet(styleSheet.readAll());
        }

        QDesktopWidget* desktop = new QDesktopWidget();
        QRect screen = desktop->screenGeometry();
        resize(static_cast<int>(screen.width() * INITIAL_WIDTH_RATIO), static_cast<int>(screen.height() * INITIAL_HEIGHT_RATIO));
        move(screen.center() - rect().center());
        delete desktop;
}

LogDialog::~LogDialog() {
    deleteLater();
    delete _logTextBox;
}

void LogDialog::showEvent(QShowEvent *e)  {
    _logTextBox->clear();
    pthread_mutex_lock(& _mutex);
    char** _lines = LogDisplay::instance.getLogData();
    char** _lastLinePos = LogDisplay::instance.getLastLinePos();

    int i = 0;
    while (_lines[i] != *_lastLinePos) {
        appendLogLine(_lines[i]);
        i++;
    }

    connect(&LogDisplay::instance, &LogDisplay::logReceived, this, &LogDialog::appendLogLine);
    pthread_mutex_unlock(& _mutex);
}

void LogDialog::resizeEvent(QResizeEvent *e) {
    _logTextBox->resize(width(), height());
}

void LogDialog::appendLogLine(QString logLine) {
    if (isVisible()) {
        _logTextBox->appendPlainText(logLine.simplified());
        _logTextBox->ensureCursorVisible();
    }
}

void LogDialog::reject() {
    close();
}

void LogDialog::closeEvent(QCloseEvent* event) {
    emit closed();
}
