//
//  LogViewer.cpp
//  StackManagerQt/src/ui
//
//  Created by Mohammed Nafees on 07/10/14.
//  Copyright (c) 2014 High Fidelity. All rights reserved.
//

#include "LogViewer.h"
#include "GlobalData.h"

#include <QTextCursor>
#include <QLabel>
#include <QVBoxLayout>

LogViewer::LogViewer(QWidget* parent) :
    QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout;
    QLabel* outputLabel = new QLabel;
    outputLabel->setText("Standard Output:");
    outputLabel->setStyleSheet("font-size: 13pt;");

    layout->addWidget(outputLabel);

    _outputView = new QTextEdit;
    _outputView->setUndoRedoEnabled(false);
    _outputView->setReadOnly(true);

    layout->addWidget(_outputView);

    QLabel* errorLabel = new QLabel;
    errorLabel->setText("Standard Error:");
    errorLabel->setStyleSheet("font-size: 13pt;");

    layout->addWidget(errorLabel);

    _errorView = new QTextEdit;
    _errorView->setUndoRedoEnabled(false);
    _errorView->setReadOnly(true);

    layout->addWidget(_errorView);
    setLayout(layout);
}

void LogViewer::clear() {
    _outputView->clear();
    _errorView->clear();
}

void LogViewer::appendStandardOutput(const QString& output) {
    QTextCursor cursor = _outputView->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(output);
    _outputView->ensureCursorVisible();
}

void LogViewer::appendStandardError(const QString& error) {
    QTextCursor cursor = _errorView->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(error);
    _errorView->ensureCursorVisible();
}
