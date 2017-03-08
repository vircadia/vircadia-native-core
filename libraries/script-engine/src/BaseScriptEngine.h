//
//  BaseScriptEngine.h
//  libraries/script-engine/src
//
//  Created by Timothy Dedischew on 02/01/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BaseScriptEngine_h
#define hifi_BaseScriptEngine_h

#include <functional>
#include <QtCore/QDebug>
#include <QtScript/QScriptEngine>

#include "SettingHandle.h"

// common base class for extending QScriptEngine itself
class BaseScriptEngine : public QScriptEngine {
    Q_OBJECT
public:
    static const QString SCRIPT_EXCEPTION_FORMAT;
    static const QString SCRIPT_BACKTRACE_SEP;

    BaseScriptEngine() {}

    Q_INVOKABLE QScriptValue evaluateInClosure(const QScriptValue& locals, const QScriptProgram& program);

    Q_INVOKABLE QScriptValue lintScript(const QString& sourceCode, const QString& fileName, const int lineNumber = 1);
    Q_INVOKABLE QScriptValue makeError(const QScriptValue& other = QScriptValue(), const QString& type = "Error");
    Q_INVOKABLE QString formatException(const QScriptValue& exception);
    QScriptValue cloneUncaughtException(const QString& detail = QString());

signals:
    void unhandledException(const QScriptValue& exception);

protected:
    void _emitUnhandledException(const QScriptValue& exception);
    QScriptValue newLambdaFunction(std::function<QScriptValue(QScriptContext *context, QScriptEngine* engine)> operation, const QScriptValue& data = QScriptValue(), const QScriptEngine::ValueOwnership& ownership = QScriptEngine::AutoOwnership);

    static const QString _SETTINGS_ENABLE_EXTENDED_EXCEPTIONS;
    Setting::Handle<bool> _enableExtendedJSExceptions { _SETTINGS_ENABLE_EXTENDED_EXCEPTIONS, true };
#ifdef DEBUG_JS
    static void _debugDump(const QString& header, const QScriptValue& object, const QString& footer = QString());
#endif
};

// Lambda helps create callable QScriptValues out of std::functions:
// (just meant for use from within the script engine itself)
class Lambda : public QObject {
    Q_OBJECT
public:
    Lambda(QScriptEngine *engine, std::function<QScriptValue(QScriptContext *context, QScriptEngine* engine)> operation, QScriptValue data);
    ~Lambda();
    public slots:
        QScriptValue call();
    QString toString() const;
private:
    QScriptEngine* engine;
    std::function<QScriptValue(QScriptContext *context, QScriptEngine* engine)> operation;
    QScriptValue data;
};

#endif // hifi_BaseScriptEngine_h
