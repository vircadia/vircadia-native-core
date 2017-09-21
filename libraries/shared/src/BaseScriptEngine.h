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

class ScriptEngine;
using ScriptEnginePointer = QSharedPointer<ScriptEngine>;

// common base class for extending QScriptEngine itself
class BaseScriptEngine : public QScriptEngine, public QEnableSharedFromThis<BaseScriptEngine> {
    Q_OBJECT
public:
    static const QString SCRIPT_EXCEPTION_FORMAT;
    static const QString SCRIPT_BACKTRACE_SEP;

    // threadsafe "unbound" version of QScriptEngine::nullValue()
    static const QScriptValue unboundNullValue() { return QScriptValue(0, QScriptValue::NullValue); }

    BaseScriptEngine() {}

    Q_INVOKABLE QScriptValue lintScript(const QString& sourceCode, const QString& fileName, const int lineNumber = 1);
    Q_INVOKABLE QScriptValue makeError(const QScriptValue& other = QScriptValue(), const QString& type = "Error");
    Q_INVOKABLE QString formatException(const QScriptValue& exception, bool includeExtendedDetails);

    QScriptValue cloneUncaughtException(const QString& detail = QString());
    QScriptValue evaluateInClosure(const QScriptValue& locals, const QScriptProgram& program);

    // if there is a pending exception and we are at the top level (non-recursive) stack frame, this emits and resets it
    bool maybeEmitUncaughtException(const QString& debugHint = QString());

    // if the currentContext() is valid then throw the passed exception; otherwise, immediately emit it.
    // note: this is used in cases where C++ code might call into JS API methods directly
    bool raiseException(const QScriptValue& exception);

    // helper to detect and log warnings when other code invokes QScriptEngine/BaseScriptEngine in thread-unsafe ways
    static bool IS_THREADSAFE_INVOCATION(const QThread *thread, const QString& method);
signals:
    void unhandledException(const QScriptValue& exception);

protected:
    // like `newFunction`, but allows mapping inline C++ lambdas with captures as callable QScriptValues
    // even though the context/engine parameters are redundant in most cases, the function signature matches `newFunction`
    // anyway so that newLambdaFunction can be used to rapidly prototype / test utility APIs and then if becoming
    // permanent more easily promoted into regular static newFunction scenarios.
    QScriptValue newLambdaFunction(std::function<QScriptValue(QScriptContext *context, QScriptEngine* engine)> operation, const QScriptValue& data = QScriptValue(), const QScriptEngine::ValueOwnership& ownership = QScriptEngine::AutoOwnership);

#ifdef DEBUG_JS
    static void _debugDump(const QString& header, const QScriptValue& object, const QString& footer = QString());
#endif
};

// Standardized CPS callback helpers (see: http://fredkschott.com/post/2014/03/understanding-error-first-callbacks-in-node-js/)
// These two helpers allow async JS APIs that use a callback parameter to be more friendly to scripters by accepting thisObject
// context and adopting a consistent and intuitable callback signature:
//   function callback(err, result) { if (err) { ... } else { /* do stuff with result */ } }
//
// To use, first pass the user-specified callback args in the same order used with optionally-scoped  Qt signal connections:
//   auto handler = makeScopedHandlerObject(scopeOrCallback, optionalMethodOrName);
// And then invoke the scoped handler later per CPS conventions:
//   auto result = callScopedHandlerObject(handler, err, result);
QScriptValue makeScopedHandlerObject(QScriptValue scopeOrCallback, QScriptValue methodOrName);
QScriptValue callScopedHandlerObject(QScriptValue handler, QScriptValue err, QScriptValue result);

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
