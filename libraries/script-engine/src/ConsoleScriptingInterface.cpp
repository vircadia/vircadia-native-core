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

#define INDENTATION 4 // 1 Tab - 4 spaces
const QString LINE_SEPARATOR = "\n    ";
const QString SPACE_SEPARATOR = " ";
const QString STACK_TRACE_FORMAT = "\n[Stacktrace]%1%2";
QList<QString> ConsoleScriptingInterface::_groupDetails = QList<QString>();

QScriptValue ConsoleScriptingInterface::info(QScriptContext* context, QScriptEngine* engine) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine)) {
        scriptEngine->scriptInfoMessage(appendArguments(context));
    }
    return QScriptValue::NullValue;
}

QScriptValue ConsoleScriptingInterface::log(QScriptContext* context, QScriptEngine* engine) {
    QString message = appendArguments(context);
    if (_groupDetails.count() == 0) {
        if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine)) {
            scriptEngine->scriptPrintedMessage(message);
        }
    } else {
        logGroupMessage(message, engine);
    }
    return QScriptValue::NullValue;
}

QScriptValue ConsoleScriptingInterface::debug(QScriptContext* context, QScriptEngine* engine) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine)) {
        scriptEngine->scriptPrintedMessage(appendArguments(context));
    }
    return QScriptValue::NullValue;
}

QScriptValue ConsoleScriptingInterface::warn(QScriptContext* context, QScriptEngine* engine) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine)) {
        scriptEngine->scriptWarningMessage(appendArguments(context));
    }
    return QScriptValue::NullValue;
}

QScriptValue ConsoleScriptingInterface::error(QScriptContext* context, QScriptEngine* engine) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine)) {
        scriptEngine->scriptErrorMessage(appendArguments(context));
    }
    return QScriptValue::NullValue;
}

QScriptValue ConsoleScriptingInterface::exception(QScriptContext* context, QScriptEngine* engine) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine)) {
        scriptEngine->scriptErrorMessage(appendArguments(context));
    }
    return QScriptValue::NullValue;
}

void ConsoleScriptingInterface::time(QString labelName) {
    _timerDetails.insert(labelName, QDateTime::currentDateTime().toUTC());
    QString message = QString("%1: Timer started").arg(labelName);
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptPrintedMessage(message);
    }
}

void ConsoleScriptingInterface::timeEnd(QString labelName) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        if (!_timerDetails.contains(labelName)) {
            scriptEngine->scriptErrorMessage("No such label found " + labelName);
            return;
        }

        if (_timerDetails.value(labelName).isNull()) {
            _timerDetails.remove(labelName);
            scriptEngine->scriptErrorMessage("Invalid start time for " + labelName);
            return;
        }
        QDateTime _startTime = _timerDetails.value(labelName);
        QDateTime _endTime = QDateTime::currentDateTime().toUTC();
        qint64 diffInMS = _startTime.msecsTo(_endTime);

        QString message = QString("%1: %2ms").arg(labelName).arg(QString::number(diffInMS));
        _timerDetails.remove(labelName);

        scriptEngine->scriptPrintedMessage(message);
    }
}

QScriptValue ConsoleScriptingInterface::assertion(QScriptContext* context, QScriptEngine* engine) {
    QString message;
    bool condition = false;
    for (int i = 0; i < context->argumentCount(); i++) {
        if (i == 0) {
            condition = context->argument(i).toBool(); // accept first value as condition
        } else {
            message += SPACE_SEPARATOR + context->argument(i).toString(); // accept other parameters as message
        }
    }

    QString assertionResult;
    if (!condition) {
        if (message.isEmpty()) {
            assertionResult = "Assertion failed";
        } else {
            assertionResult = QString("Assertion failed : %1").arg(message);
        }
        if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine)) {
            scriptEngine->scriptErrorMessage(assertionResult);
        }
    }
    return QScriptValue::NullValue;
}

void ConsoleScriptingInterface::trace() {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptPrintedMessage
            (QString(STACK_TRACE_FORMAT).arg(LINE_SEPARATOR,
            scriptEngine->currentContext()->backtrace().join(LINE_SEPARATOR)));
    }
}

void ConsoleScriptingInterface::clear() {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->clearDebugLogWindow();
    }
}

QScriptValue ConsoleScriptingInterface::group(QScriptContext* context, QScriptEngine* engine) {
    logGroupMessage(context->argument(0).toString(), engine); // accept first parameter as label
    _groupDetails.push_back(context->argument(0).toString());
    return QScriptValue::NullValue;
}

QScriptValue ConsoleScriptingInterface::groupCollapsed(QScriptContext* context, QScriptEngine* engine) {
    logGroupMessage(context->argument(0).toString(), engine); // accept first parameter as label
    _groupDetails.push_back(context->argument(0).toString());
    return QScriptValue::NullValue;
}

QScriptValue ConsoleScriptingInterface::groupEnd(QScriptContext* context, QScriptEngine* engine) {
    ConsoleScriptingInterface::_groupDetails.removeLast();
    return QScriptValue::NullValue;
}

QString ConsoleScriptingInterface::appendArguments(QScriptContext* context) {
    QString message;
    for (int i = 0; i < context->argumentCount(); i++) {
        if (i > 0) {
            message += SPACE_SEPARATOR;
        }
        message += context->argument(i).toString();
    }
    return message;
}

void ConsoleScriptingInterface::logGroupMessage(QString message, QScriptEngine* engine) {
    int _addSpaces = _groupDetails.count() * INDENTATION;
    QString logMessage;
    for (int i = 0; i < _addSpaces; i++) {
        logMessage.append(SPACE_SEPARATOR);
    }
    logMessage.append(message);
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine)) {
        scriptEngine->scriptPrintedMessage(logMessage);
    }
}
