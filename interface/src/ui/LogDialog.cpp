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

#define INITIAL_WIDTH_RATIO 0.3
#define INITIAL_HEIGHT_RATIO 0.6

int cursorMeta = qRegisterMetaType<QTextCursor>("QTextCursor");
int blockMeta = qRegisterMetaType<QTextBlock>("QTextBlock");

LogDialog::LogDialog(QWidget* parent) :
    QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint) {

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
        resize((int)(screen.width() * INITIAL_WIDTH_RATIO), (int)(screen.height() * INITIAL_HEIGHT_RATIO));
        move(screen.center() - rect().center());
        delete desktop;
}

LogDialog::~LogDialog() {
    delete _logTextBox;
}

void LogDialog::showEvent(QShowEvent *e)  {
    _logTextBox->clear();
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
    this->QDialog::close();
}

void LogDialog::closeEvent(QCloseEvent* event) {
    emit closed();
}
