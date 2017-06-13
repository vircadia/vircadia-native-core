//
//  ConsoleScriptingInterface.cpp
//  libraries/script-engine/src
//
//  Created by NeetBhagat on 6/1/17.
//  Copyright 2014 High Fidelity, Inc.
//
//  ConsoleScriptingInterface is responsible for following functionality
//  Printing logs with various tags and grouping on debug Window and Logs/log file.
//  Debugging functionalities like Timer start-end, assertion, trace.
//  To use these functionalities, use "console" object in Javascript files.
//  For examples please refer "scripts/developer/tests/consoleObjectTest.js"
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ConsoleScriptingInterface.h"
#include "ScriptEngine.h"

#define INDENTATION 4
const QString LINE_SEPARATOR = "\n    ";
const QString STACK_TRACE_FORMAT = "\n[Stacktrace]%1%2";

void ConsoleScriptingInterface::info(QString message) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptInfoMessage(message);
    }
}

void ConsoleScriptingInterface::log(QString message) {
    if (_groupDetails.count() == 0) {
        if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
            scriptEngine->scriptPrintedMessage(message);
        }
    } else {
        this->logGroupMessage(message);
    }
}

void ConsoleScriptingInterface::debug(QString message) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptPrintedMessage(message);
    }
}

void ConsoleScriptingInterface::warn(QString message) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptWarningMessage(message);
    }
}

void ConsoleScriptingInterface::error(QString message) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptErrorMessage(message);
    }
}

void ConsoleScriptingInterface::exception(QString message) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptErrorMessage(message);
    }
}

void  ConsoleScriptingInterface::time(QString labelName) {
    _timerDetails.insert(labelName, QDateTime::currentDateTime().toUTC());
    QString message = QString("%1: timer started").arg(labelName);
    this->log(message);
}

void ConsoleScriptingInterface::timeEnd(QString labelName) {
    if (!_timerDetails.contains(labelName)) {
        this->error("No such label found " + labelName);
        return;
    }

    if (_timerDetails.value(labelName).isNull()) {
        _timerDetails.remove(labelName);
        this->error("Invalid start time for " + labelName);
        return;
    }
    
    QDateTime _startTime = _timerDetails.value(labelName);
    QDateTime _endTime = QDateTime::currentDateTime().toUTC();
    qint64 diffInMS = _startTime.msecsTo(_endTime);
    
    QString message = QString("%1: %2ms").arg(labelName).arg(QString::number(diffInMS));
    _timerDetails.remove(labelName);

    this->log(message);
}

void  ConsoleScriptingInterface::asserts(bool condition, QString message) {
    if (!condition) {
        QString assertionResult;
        if (message.isEmpty()) {
            assertionResult = "Assertion failed";
        } else {
            assertionResult = QString("Assertion failed : %1").arg(message);
        }
        this->error(assertionResult);
    }
}

void  ConsoleScriptingInterface::trace() {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {        
        scriptEngine->scriptPrintedMessage
            (QString(STACK_TRACE_FORMAT).arg(LINE_SEPARATOR,
            scriptEngine->currentContext()->backtrace().join(LINE_SEPARATOR)));
    }
}

void  ConsoleScriptingInterface::clear() {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->clearDebugLogWindow();
    }
}

void ConsoleScriptingInterface::group(QString groupName) {
    this->logGroupMessage(groupName);
    _groupDetails.push_back(groupName);
}

void ConsoleScriptingInterface::groupCollapsed(QString  groupName) {
    this->logGroupMessage(groupName);
    _groupDetails.push_back(groupName);
}

void ConsoleScriptingInterface::groupEnd() {
    _groupDetails.removeLast();
}

void ConsoleScriptingInterface::logGroupMessage(QString message) {
    int _appendIndentation = _groupDetails.count() * INDENTATION;
    QString logMessage;
    for (int count = 0; count < _appendIndentation; count++) {
        logMessage.append(" ");
    }
    logMessage.append(message);
    this->debug(logMessage);
}
