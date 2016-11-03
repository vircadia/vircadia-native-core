//
//  JSConsole.cpp
//  interface/src/ui
//
//  Created by Ryan Huffman on 05/12/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QFuture>
#include <QKeyEvent>
#include <QLabel>
#include <QScrollBar>
#include <QtConcurrent/QtConcurrentRun>

#include <ScriptEngines.h>
#include <PathUtils.h>

#include "Application.h"
#include "JSConsole.h"
#include "ScriptHighlighting.h"

const int NO_CURRENT_HISTORY_COMMAND = -1;
const int MAX_HISTORY_SIZE = 64;

const QString COMMAND_STYLE = "color: #266a9b;";

const QString RESULT_SUCCESS_STYLE = "color: #677373;";
const QString RESULT_ERROR_STYLE = "color: #d13b22;";

const QString GUTTER_PREVIOUS_COMMAND = "<span style=\"color: #57b8bb;\">&lt;</span>";
const QString GUTTER_ERROR = "<span style=\"color: #d13b22;\">X</span>";

JSConsole::JSConsole(QWidget* parent, ScriptEngine* scriptEngine) :
    QWidget(parent),
    _ui(new Ui::Console),
    _currentCommandInHistory(NO_CURRENT_HISTORY_COMMAND),
    _commandHistory(),
    _ownScriptEngine(scriptEngine == NULL),
    _scriptEngine(NULL) {

    _ui->setupUi(this);
    _ui->promptTextEdit->setLineWrapMode(QTextEdit::NoWrap);
    _ui->promptTextEdit->setWordWrapMode(QTextOption::NoWrap);
    _ui->promptTextEdit->installEventFilter(this);

    QFile styleSheet(PathUtils::resourcesPath() + "styles/console.qss");
    if (styleSheet.open(QIODevice::ReadOnly)) {
        QDir::setCurrent(PathUtils::resourcesPath());
        setStyleSheet(styleSheet.readAll());
    }

    connect(_ui->scrollArea->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(scrollToBottom()));
    connect(_ui->promptTextEdit, SIGNAL(textChanged()), this, SLOT(resizeTextInput()));

    setScriptEngine(scriptEngine);

    resizeTextInput();

    connect(&_executeWatcher, SIGNAL(finished()), this, SLOT(commandFinished()));
}

JSConsole::~JSConsole() {
    disconnect(_scriptEngine, SIGNAL(printedMessage(const QString&)), this, SLOT(handlePrint(const QString&)));
    disconnect(_scriptEngine, SIGNAL(errorMessage(const QString&)), this, SLOT(handleError(const QString&)));
    if (_ownScriptEngine) {
        _scriptEngine->deleteLater();
    }
    delete _ui;
}

void JSConsole::setScriptEngine(ScriptEngine* scriptEngine) {
    if (_scriptEngine == scriptEngine && scriptEngine != NULL) {
        return;
    }
    if (_scriptEngine != NULL) {
        disconnect(_scriptEngine, SIGNAL(printedMessage(const QString&)), this, SLOT(handlePrint(const QString&)));
        disconnect(_scriptEngine, SIGNAL(errorMessage(const QString&)), this, SLOT(handleError(const QString&)));
        if (_ownScriptEngine) {
            _scriptEngine->deleteLater();
        }
    }

    // if scriptEngine is NULL then create one and keep track of it using _ownScriptEngine
    _ownScriptEngine = scriptEngine == NULL;
    _scriptEngine = _ownScriptEngine ? DependencyManager::get<ScriptEngines>()->loadScript(QString(), false) : scriptEngine;

    connect(_scriptEngine, SIGNAL(printedMessage(const QString&)), this, SLOT(handlePrint(const QString&)));
    connect(_scriptEngine, SIGNAL(errorMessage(const QString&)), this, SLOT(handleError(const QString&)));
}

void JSConsole::executeCommand(const QString& command) {
    _commandHistory.prepend(command);
    if (_commandHistory.length() > MAX_HISTORY_SIZE) {
        _commandHistory.removeLast();
    }

    _ui->promptTextEdit->setDisabled(true);

    appendMessage(">", "<span style='" + COMMAND_STYLE + "'>" + command.toHtmlEscaped() + "</span>");

    QFuture<QScriptValue> future = QtConcurrent::run(this, &JSConsole::executeCommandInWatcher, command);
    _executeWatcher.setFuture(future);
}

QScriptValue JSConsole::executeCommandInWatcher(const QString& command) {
    QScriptValue result;
    static const QString filename = "JSConcole";
    QMetaObject::invokeMethod(_scriptEngine, "evaluate", Qt::ConnectionType::BlockingQueuedConnection,
                              Q_RETURN_ARG(QScriptValue, result),
                              Q_ARG(const QString&, command),
                              Q_ARG(const QString&, filename));
    return result;
}

void JSConsole::commandFinished() {
    QScriptValue result = _executeWatcher.result();

    _ui->promptTextEdit->setDisabled(false);

    // Make sure focus is still on this window - some commands are blocking and can take awhile to execute.
    if (window()->isActiveWindow()) {
        _ui->promptTextEdit->setFocus();
    }

    bool error = (_scriptEngine->hasUncaughtException() || result.isError());
    QString gutter = error ? GUTTER_ERROR : GUTTER_PREVIOUS_COMMAND;
    QString resultColor = error ? RESULT_ERROR_STYLE : RESULT_SUCCESS_STYLE;
    QString resultStr = "<span style='" + resultColor + "'>" + result.toString().toHtmlEscaped() + "</span>";
    appendMessage(gutter, resultStr);

    resetCurrentCommandHistory();
}

void JSConsole::handleError(const QString& message) {
    appendMessage(GUTTER_ERROR, "<span style='" + RESULT_ERROR_STYLE + "'>" + message.toHtmlEscaped() + "</span>");
}

void JSConsole::handlePrint(const QString& message) {
    appendMessage("", message);
}

void JSConsole::mouseReleaseEvent(QMouseEvent* event) {
    _ui->promptTextEdit->setFocus();
}

void JSConsole::showEvent(QShowEvent* event) {
    _ui->promptTextEdit->setFocus();
}

bool JSConsole::eventFilter(QObject* sender, QEvent* event) {
    if (sender == _ui->promptTextEdit) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            int key = keyEvent->key();

            if ((key == Qt::Key_Return || key == Qt::Key_Enter)) {
                if (keyEvent->modifiers() & Qt::ShiftModifier) {
                    // If the shift key is being used then treat it as a regular return/enter.  If this isn't done,
                    // a new QTextBlock isn't created.
                    keyEvent->setModifiers(keyEvent->modifiers() & ~Qt::ShiftModifier);
                } else {
                    QString command = _ui->promptTextEdit->toPlainText().trimmed();

                    if (!command.isEmpty()) {
                        QTextCursor cursor = _ui->promptTextEdit->textCursor();
                        _ui->promptTextEdit->clear();

                        executeCommand(command);
                    }

                    return true;
                }
            } else if (key == Qt::Key_Down) {
                // Go to the next command in history if the cursor is at the last line of the current command.
                int blockNumber = _ui->promptTextEdit->textCursor().blockNumber();
                int blockCount = _ui->promptTextEdit->document()->blockCount();
                if (blockNumber == blockCount - 1) {
                    setToNextCommandInHistory();
                    return true;
                }
            } else if (key == Qt::Key_Up) {
                // Go to the previous command in history if the cursor is at the first line of the current command.
                int blockNumber = _ui->promptTextEdit->textCursor().blockNumber();
                if (blockNumber == 0) {
                    setToPreviousCommandInHistory();
                    return true;
                }
            }
        }
    }
    return false;
}

void JSConsole::setToNextCommandInHistory() {
    if (_currentCommandInHistory >= 0) {
        _currentCommandInHistory--;
        if (_currentCommandInHistory == NO_CURRENT_HISTORY_COMMAND) {
            setAndSelectCommand(_rootCommand);
        } else {
            setAndSelectCommand(_commandHistory[_currentCommandInHistory]);
        }
    }
}

void JSConsole::setToPreviousCommandInHistory() {
    if (_currentCommandInHistory < (_commandHistory.length() - 1)) {
        if (_currentCommandInHistory == NO_CURRENT_HISTORY_COMMAND) {
            _rootCommand = _ui->promptTextEdit->toPlainText();
        }
        _currentCommandInHistory++;
        setAndSelectCommand(_commandHistory[_currentCommandInHistory]);
    }
}

void JSConsole::resetCurrentCommandHistory() {
    _currentCommandInHistory = NO_CURRENT_HISTORY_COMMAND;
}

void JSConsole::resizeTextInput() {
    _ui->promptTextEdit->setFixedHeight(_ui->promptTextEdit->document()->size().height());
    _ui->promptTextEdit->updateGeometry();
}

void JSConsole::setAndSelectCommand(const QString& text) {
    QTextCursor cursor = _ui->promptTextEdit->textCursor();
    cursor.select(QTextCursor::Document);
    cursor.deleteChar();
    cursor.insertText(text);
    cursor.movePosition(QTextCursor::End);
}

void JSConsole::scrollToBottom() {
    QScrollBar* scrollBar = _ui->scrollArea->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void JSConsole::appendMessage(const QString& gutter, const QString& message) {
    QWidget* logLine = new QWidget(_ui->logArea);
    QHBoxLayout* layout = new QHBoxLayout(logLine);
    layout->setMargin(0);
    layout->setSpacing(4);

    QLabel* gutterLabel = new QLabel(logLine);
    QLabel* messageLabel = new QLabel(logLine);

    gutterLabel->setFixedWidth(16);
    gutterLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    gutterLabel->setStyleSheet("font-size: 14px; font-family: Inconsolata, Lucida Console, Andale Mono, Monaco;");
    messageLabel->setStyleSheet("font-size: 14px; font-family: Inconsolata, Lucida Console, Andale Mono, Monaco;");

    gutterLabel->setText(gutter);
    messageLabel->setText(message);

    layout->addWidget(gutterLabel);
    layout->addWidget(messageLabel);
    logLine->setLayout(layout);

    layout->setAlignment(gutterLabel, Qt::AlignTop);

    layout->setStretch(0, 0);
    layout->setStretch(1, 1);

    _ui->logArea->layout()->addWidget(logLine);

    _ui->logArea->updateGeometry();
    scrollToBottom();
}

void JSConsole::clear() {
    QLayoutItem* item;
    while ((item = _ui->logArea->layout()->takeAt(0)) != NULL) {
        delete item->widget();
        delete item;
    }
    _ui->logArea->updateGeometry();
    scrollToBottom();
}
