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

const int INITIAL_WIDTH  = 720;
const float INITIAL_HEIGHT_RATIO = 0.6f;

int cursorMeta = qRegisterMetaType<QTextCursor>("QTextCursor");
int blockMeta = qRegisterMetaType<QTextBlock>("QTextBlock");

LogDialog::LogDialog(QWidget* parent) : QDialog(parent, Qt::Dialog) {

    setWindowTitle("Log");

    _logTextBox = new QPlainTextEdit(this);
    _logTextBox->setReadOnly(true);
    _logTextBox->show();

    switchToResourcesParentIfRequired();
    QFile styleSheet("resources/styles/log_dialog.qss");

    if (styleSheet.open(QIODevice::ReadOnly)) {
        setStyleSheet(styleSheet.readAll());
    }

    QDesktopWidget desktop;
    QRect screen = desktop.screenGeometry();
    resize(INITIAL_WIDTH, static_cast<int>(screen.height() * INITIAL_HEIGHT_RATIO));
    move(screen.center() - rect().center());

    setAttribute(Qt::WA_DeleteOnClose);
}

LogDialog::~LogDialog() {
    deleteLater();
}

void LogDialog::showEvent(QShowEvent *e)  {
    _logTextBox->clear();

    pthread_mutex_lock(& _mutex);
    QStringList _logData = LogDisplay::instance.getLogData();

    connect(&LogDisplay::instance, &LogDisplay::logReceived, this, &LogDialog::appendLogLine);
    for(int i = 0; i < _logData.size(); ++i) {
        appendLogLine(_logData[i]);
    }

    pthread_mutex_unlock(& _mutex);
}

void LogDialog::resizeEvent(QResizeEvent *e) {
    _logTextBox->resize(width(), height());
}

void LogDialog::appendLogLine(QString logLine) {
    if (isVisible()) {
        pthread_mutex_lock(& _mutex);
        _logTextBox->appendPlainText(logLine.simplified());
        pthread_mutex_unlock(& _mutex);
        _logTextBox->ensureCursorVisible();
    }
}
