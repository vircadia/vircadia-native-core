//
//  ScriptEngineQtScript.cpp
//  libraries/script-engine/src/qtscript
//
//  Created by Brad Hefta-Gaub on 12/14/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptEngineQtScript.h"

#include <chrono>
#include <thread>

#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtCore/QRegularExpression>

#include <QtCore/QFuture>
#include <QtConcurrent/QtConcurrentRun>

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>

#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <QtScript/QScriptContextInfo>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptValueIterator>

#include <QtScriptTools/QScriptEngineDebugger>

#include <shared/LocalFileAccessGate.h>
#include <shared/QtHelpers.h>
#include <shared/AbstractLoggerInterface.h>

#include <Profile.h>

#include "../ScriptEngineLogging.h"
#include "../ScriptProgram.h"
#include "../ScriptValue.h"

#include "ScriptContextQtWrapper.h"
#include "ScriptObjectQtProxy.h"
#include "ScriptProgramQtWrapper.h"
#include "ScriptValueQtWrapper.h"

static const int MAX_DEBUG_VALUE_LENGTH { 80 };

bool ScriptEngineQtScript::IS_THREADSAFE_INVOCATION(const QThread* thread, const QString& method) {
    const QThread* currentThread = QThread::currentThread();
    if (currentThread == thread) {
        return true;
    }
    qCCritical(scriptengine) << QString("Scripting::%1 @ %2 -- ignoring thread-unsafe call from %3")
                              .arg(method)
                              .arg(thread ? thread->objectName() : "(!thread)")
                              .arg(QThread::currentThread()->objectName());
    qCDebug(scriptengine) << "(please resolve on the calling side by using invokeMethod, executeOnScriptThread, etc.)";
    Q_ASSERT(false);
    return false;
}

// engine-aware JS Error copier and factory
QScriptValue ScriptEngineQtScript::makeError(const QScriptValue& _other, const QString& type) {
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return QScriptEngine::nullValue();
    }
    auto other = _other;
    if (other.isString()) {
        other = QScriptEngine::newObject();
        other.setProperty("message", _other.toString());
    }
    auto proto = QScriptEngine::globalObject().property(type);
    if (!proto.isFunction()) {
        proto = QScriptEngine::globalObject().property(other.prototype().property("constructor").property("name").toString());
    }
    if (!proto.isFunction()) {
#ifdef DEBUG_JS_EXCEPTIONS
        qCDebug(shared) << "BaseScriptEngine::makeError -- couldn't find constructor for" << type << " -- using Error instead";
#endif
        proto = QScriptEngine::globalObject().property("Error");
    }
    if (other.engine() != this) {
        // JS Objects are parented to a specific script engine instance
        // -- this effectively ~clones it locally by routing through a QVariant and back
        other = QScriptEngine::toScriptValue(other.toVariant());
    }
    // ~ var err = new Error(other.message)
    auto err = proto.construct(QScriptValueList({ other.property("message") }));

    // transfer over any existing properties
    QScriptValueIterator it(other);
    while (it.hasNext()) {
        it.next();
        err.setProperty(it.name(), it.value());
    }
    return err;
}

ScriptValue ScriptEngineQtScript::makeError(const ScriptValue& _other, const QString& type) {
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return nullValue();
    }
    ScriptValueQtWrapper* unwrapped = ScriptValueQtWrapper::unwrap(_other);
    QScriptValue other;
    if (_other.isString()) {
        other = QScriptEngine::newObject();
        other.setProperty("message", _other.toString());
    } else if (unwrapped) {
        other = unwrapped->toQtValue();
    } else {
        other = QScriptEngine::newVariant(_other.toVariant());
    }
    QScriptValue result = makeError(other, type);
    return ScriptValue(new ScriptValueQtWrapper(this, std::move(result)));
}

// check syntax and when there are issues returns an actual "SyntaxError" with the details
ScriptValue ScriptEngineQtScript::lintScript(const QString& sourceCode, const QString& fileName, const int lineNumber) {
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return nullValue();
    }
    const auto syntaxCheck = checkSyntax(sourceCode);
    if (syntaxCheck.state() != QScriptSyntaxCheckResult::Valid) {
        auto err = QScriptEngine::globalObject().property("SyntaxError").construct(QScriptValueList({ syntaxCheck.errorMessage() }));
        err.setProperty("fileName", fileName);
        err.setProperty("lineNumber", syntaxCheck.errorLineNumber());
        err.setProperty("expressionBeginOffset", syntaxCheck.errorColumnNumber());
        err.setProperty("stack", currentContext()->backtrace().join(ScriptManager::SCRIPT_BACKTRACE_SEP));
        {
            const auto error = syntaxCheck.errorMessage();
            const auto line = QString::number(syntaxCheck.errorLineNumber());
            const auto column = QString::number(syntaxCheck.errorColumnNumber());
            // for compatibility with legacy reporting
            const auto message = QString("[SyntaxError] %1 in %2:%3(%4)").arg(error, fileName, line, column);
            err.setProperty("formatted", message);
        }
        return ScriptValue(new ScriptValueQtWrapper(this, std::move(err)));
    }
    return undefinedValue();
}

// this pulls from the best available information to create a detailed snapshot of the current exception
ScriptValue ScriptEngineQtScript::cloneUncaughtException(const QString& extraDetail) {
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return nullValue();
    }
    if (!hasUncaughtException()) {
        return nullValue();
    }
    auto exception = uncaughtException();
    // ensure the error object is engine-local
    auto err = makeError(exception);

    // not sure why Qt does't offer uncaughtExceptionFileName -- but the line number
    // on its own is often useless/wrong if arbitrarily married to a filename.
    // when the error object already has this info, it seems to be the most reliable
    auto fileName = exception.property("fileName").toString();
    auto lineNumber = exception.property("lineNumber").toInt32();

    // the backtrace, on the other hand, seems most reliable taken from uncaughtExceptionBacktrace
    auto backtrace = uncaughtExceptionBacktrace();
    if (backtrace.isEmpty()) {
        // fallback to the error object
        backtrace = exception.property("stack").toString().split(ScriptManager::SCRIPT_BACKTRACE_SEP);
    }
    // the ad hoc "detail" property can be used now to embed additional clues
    auto detail = exception.property("detail").toString();
    if (detail.isEmpty()) {
        detail = extraDetail;
    } else if (!extraDetail.isEmpty()) {
        detail += "(" + extraDetail + ")";
    }
    if (lineNumber <= 0) {
        lineNumber = uncaughtExceptionLineNumber();
    }
    if (fileName.isEmpty()) {
        // climb the stack frames looking for something useful to display
        for (auto c = QScriptEngine::currentContext(); c && fileName.isEmpty(); c = c->parentContext()) {
            QScriptContextInfo info{ c };
            if (!info.fileName().isEmpty()) {
                // take fileName:lineNumber as a pair
                fileName = info.fileName();
                lineNumber = info.lineNumber();
                if (backtrace.isEmpty()) {
                    backtrace = c->backtrace();
                }
                break;
            }
        }
    }
    err.setProperty("fileName", fileName);
    err.setProperty("lineNumber", lineNumber);
    err.setProperty("detail", detail);
    err.setProperty("stack", backtrace.join(ScriptManager::SCRIPT_BACKTRACE_SEP));

#ifdef DEBUG_JS_EXCEPTIONS
    err.setProperty("_fileName", exception.property("fileName").toString());
    err.setProperty("_stack", uncaughtExceptionBacktrace().join(SCRIPT_BACKTRACE_SEP));
    err.setProperty("_lineNumber", uncaughtExceptionLineNumber());
#endif
    return err;
}

bool ScriptEngineQtScript::raiseException(const QScriptValue& exception) {
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return false;
    }
    if (QScriptEngine::currentContext()) {
        // we have an active context / JS stack frame so throw the exception per usual
        QScriptEngine::currentContext()->throwValue(makeError(exception));
        return true;
    } else if (_scriptManager) {
        // we are within a pure C++ stack frame (ie: being called directly by other C++ code)
        // in this case no context information is available so just emit the exception for reporting
        QScriptValue thrown = makeError(exception);
        emit _scriptManager->unhandledException(ScriptValue(new ScriptValueQtWrapper(this, std::move(thrown))));
    }
    return false;
}

bool ScriptEngineQtScript::maybeEmitUncaughtException(const QString& debugHint) {
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return false;
    }
    if (!isEvaluating() && hasUncaughtException() && _scriptManager) {
        emit _scriptManager->unhandledException(cloneUncaughtException(debugHint));
        clearExceptions();
        return true;
    }
    return false;
}

// Lambda
QScriptValue ScriptEngineQtScript::newLambdaFunction(std::function<QScriptValue(QScriptContext*, ScriptEngineQtScript*)> operation,
                                                 const QScriptValue& data,
                                                 const QScriptEngine::ValueOwnership& ownership) {
    auto lambda = new Lambda(this, operation, data);
    auto object = QScriptEngine::newQObject(lambda, ownership);
    auto call = object.property("call");
    call.setPrototype(object);  // context->callee().prototype() === Lambda QObject
    call.setData(data);         // context->callee().data() will === data param
    return call;
}
QString Lambda::toString() const {
    return QString("[Lambda%1]").arg(data.isValid() ? " " + data.toString() : data.toString());
}

Lambda::~Lambda() {
#ifdef DEBUG_JS_LAMBDA_FUNCS
    qDebug() << "~Lambda"
             << "this" << this;
#endif
}

Lambda::Lambda(ScriptEngineQtScript* engine,
               std::function<QScriptValue(QScriptContext*, ScriptEngineQtScript*)> operation,
               QScriptValue data) :
    engine(engine),
    operation(operation), data(data) {
#ifdef DEBUG_JS_LAMBDA_FUNCS
    qDebug() << "Lambda" << data.toString();
#endif
}
QScriptValue Lambda::call() {
    if (!engine->IS_THREADSAFE_INVOCATION(__FUNCTION__)) {
        return static_cast<QScriptEngine*>(engine)->nullValue();
    }
    return operation(static_cast<QScriptEngine*>(engine)->currentContext(), engine);
}

#ifdef DEBUG_JS
void ScriptEngineQtScript::_debugDump(const QString& header, const QScriptValue& object, const QString& footer) {
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return;
    }
    if (!header.isEmpty()) {
        qCDebug(shared) << header;
    }
    if (!object.isObject()) {
        qCDebug(shared) << "(!isObject)" << object.toVariant().toString() << object.toString();
        return;
    }
    QScriptValueIterator it(object);
    while (it.hasNext()) {
        it.next();
        qCDebug(shared) << it.name() << ":" << it.value().toString();
    }
    if (!footer.isEmpty()) {
        qCDebug(shared) << footer;
    }
}
#endif

ScriptEngineQtScript::ScriptEngineQtScript(ScriptManager* scriptManager) :
    QScriptEngine(),
    _scriptManager(scriptManager),
    _arrayBufferClass(new ArrayBufferClass(this))
{
    registerSystemTypes();

    if (_scriptManager) {
        connect(this, &QScriptEngine::signalHandlerException, this, [this](const QScriptValue& exception) {
            if (hasUncaughtException()) {
                // the engine's uncaughtException() seems to produce much better stack traces here
                emit _scriptManager->unhandledException(cloneUncaughtException("signalHandlerException"));
                clearExceptions();
            } else {
                // ... but may not always be available -- so if needed we fallback to the passed exception
                QScriptValue thrown = makeError(exception);
                emit _scriptManager->unhandledException(ScriptValue(new ScriptValueQtWrapper(this, std::move(thrown))));
            }
        }, Qt::DirectConnection);
        moveToThread(scriptManager->thread());
    }

    QScriptValue null = QScriptEngine::nullValue();
    _nullValue = ScriptValue(new ScriptValueQtWrapper(this, std::move(null)));

    QScriptValue undefined = QScriptEngine::undefinedValue();
    _undefinedValue = ScriptValue(new ScriptValueQtWrapper(this, std::move(undefined)));

    QScriptEngine::setProcessEventsInterval(MSECS_PER_SECOND);
}

void ScriptEngineQtScript::registerEnum(const QString& enumName, QMetaEnum newEnum) {
    if (!newEnum.isValid()) {
        qCCritical(scriptengine) << "registerEnum called on invalid enum with name " << enumName;
        return;
    }

    for (int i = 0; i < newEnum.keyCount(); i++) {
        const char* keyName = newEnum.key(i);
        QString fullName = enumName + "." + keyName;
        registerValue(fullName, newEnum.keyToValue(keyName));
    }
}

void ScriptEngineQtScript::registerValue(const QString& valueName, QScriptValue value) {
    if (QThread::currentThread() != QScriptEngine::thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngineQtScript::registerValue() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]";
#endif
        QMetaObject::invokeMethod(this, "registerValue",
                                  Q_ARG(const QString&, valueName),
                                  Q_ARG(QScriptValue, value));
        return;
    }

    QStringList pathToValue = valueName.split(".");
    int partsToGo = pathToValue.length();
    QScriptValue partObject = QScriptEngine::globalObject();

    for (const auto& pathPart : pathToValue) {
        partsToGo--;
        if (!partObject.property(pathPart).isValid()) {
            if (partsToGo > 0) {
                //QObject *object = new QObject;
                QScriptValue partValue = QScriptEngine::newArray();  //newQObject(object, QScriptEngine::ScriptOwnership);
                partObject.setProperty(pathPart, partValue);
            } else {
                partObject.setProperty(pathPart, value);
            }
        }
        partObject = partObject.property(pathPart);
    }
}

void ScriptEngineQtScript::registerGlobalObject(const QString& name, QObject* object) {
    if (QThread::currentThread() != QScriptEngine::thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngineQtScript::registerGlobalObject() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]  name:" << name;
#endif
        QMetaObject::invokeMethod(this, "registerGlobalObject",
                                  Q_ARG(const QString&, name),
                                  Q_ARG(QObject*, object));
        return;
    }
#ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptEngineQtScript::registerGlobalObject() called on thread [" << QThread::currentThread() << "] name:" << name;
#endif

    if (!QScriptEngine::globalObject().property(name).isValid()) {
        if (object) {
            QScriptValue value = ScriptObjectQtProxy::newQObject(this, object, ScriptEngine::QtOwnership);
            QScriptEngine::globalObject().setProperty(name, value);
        } else {
            QScriptEngine::globalObject().setProperty(name, QScriptValue());
        }
    }
}

void ScriptEngineQtScript::registerFunction(const QString& name, QScriptEngine::FunctionSignature functionSignature, int numArguments) {
    if (QThread::currentThread() != QScriptEngine::thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngineQtScript::registerFunction() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] name:" << name;
#endif
        QMetaObject::invokeMethod(this, "registerFunction",
                                  Q_ARG(const QString&, name),
                                  Q_ARG(QScriptEngine::FunctionSignature, functionSignature),
                                  Q_ARG(int, numArguments));
        return;
    }
#ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptEngineQtScript::registerFunction() called on thread [" << QThread::currentThread() << "] name:" << name;
#endif

    QScriptValue scriptFun = QScriptEngine::newFunction(functionSignature, numArguments);
    QScriptEngine::globalObject().setProperty(name, scriptFun);
}

void ScriptEngineQtScript::registerFunction(const QString& name, ScriptEngine::FunctionSignature functionSignature, int numArguments) {
    if (QThread::currentThread() != QScriptEngine::thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngineQtScript::registerFunction() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] name:" << name;
#endif
        QMetaObject::invokeMethod(this, "registerFunction",
                                  Q_ARG(const QString&, name),
                                  Q_ARG(ScriptEngine::FunctionSignature, functionSignature),
                                  Q_ARG(int, numArguments));
        return;
    }
#ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptEngineQtScript::registerFunction() called on thread [" << QThread::currentThread() << "] name:" << name;
#endif

    auto scriptFun = newFunction(functionSignature, numArguments);
    globalObject().setProperty(name, scriptFun);
}

void ScriptEngineQtScript::registerFunction(const QString& parent, const QString& name, QScriptEngine::FunctionSignature functionSignature, int numArguments) {
    if (QThread::currentThread() != QScriptEngine::thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngineQtScript::registerFunction() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] parent:" << parent << "name:" << name;
#endif
        QMetaObject::invokeMethod(this, "registerFunction",
                                  Q_ARG(const QString&, name),
                                  Q_ARG(QScriptEngine::FunctionSignature, functionSignature),
                                  Q_ARG(int, numArguments));
        return;
    }
#ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptEngineQtScript::registerFunction() called on thread [" << QThread::currentThread() << "] parent:" << parent << "name:" << name;
#endif

    QScriptValue object = QScriptEngine::globalObject().property(parent);
    if (object.isValid()) {
        QScriptValue scriptFun = QScriptEngine::newFunction(functionSignature, numArguments);
        object.setProperty(name, scriptFun);
    }
}

void ScriptEngineQtScript::registerFunction(const QString& parent, const QString& name, ScriptEngine::FunctionSignature functionSignature, int numArguments) {
    if (QThread::currentThread() != QScriptEngine::thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngineQtScript::registerFunction() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] parent:" << parent << "name:" << name;
#endif
        QMetaObject::invokeMethod(this, "registerFunction",
                                  Q_ARG(const QString&, name),
                                  Q_ARG(ScriptEngine::FunctionSignature, functionSignature),
                                  Q_ARG(int, numArguments));
        return;
    }
#ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptEngineQtScript::registerFunction() called on thread [" << QThread::currentThread() << "] parent:" << parent << "name:" << name;
#endif

    auto object = globalObject().property(parent);
    if (object.isValid()) {
        auto scriptFun = newFunction(functionSignature, numArguments);
        object.setProperty(name, scriptFun);
    }
}

void ScriptEngineQtScript::registerGetterSetter(const QString& name, QScriptEngine::FunctionSignature getter,
                                        QScriptEngine::FunctionSignature setter, const QString& parent) {
    if (QThread::currentThread() != QScriptEngine::thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngineQtScript::registerGetterSetter() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] "
            " name:" << name << "parent:" << parent;
#endif
        QMetaObject::invokeMethod(this, "registerGetterSetter",
                                  Q_ARG(const QString&, name),
                                  Q_ARG(QScriptEngine::FunctionSignature, getter),
                                  Q_ARG(QScriptEngine::FunctionSignature, setter),
                                  Q_ARG(const QString&, parent));
        return;
    }
#ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptEngineQtScript::registerGetterSetter() called on thread [" << QThread::currentThread() << "] name:" << name << "parent:" << parent;
#endif

    QScriptValue setterFunction = QScriptEngine::newFunction(setter, 1);
    QScriptValue getterFunction = QScriptEngine::newFunction(getter);

    if (!parent.isNull() && !parent.isEmpty()) {
        QScriptValue object = QScriptEngine::globalObject().property(parent);
        if (object.isValid()) {
            object.setProperty(name, setterFunction, QScriptValue::PropertySetter);
            object.setProperty(name, getterFunction, QScriptValue::PropertyGetter);
        }
    } else {
        QScriptEngine::globalObject().setProperty(name, setterFunction, QScriptValue::PropertySetter);
        QScriptEngine::globalObject().setProperty(name, getterFunction, QScriptValue::PropertyGetter);
    }
}

void ScriptEngineQtScript::registerGetterSetter(const QString& name, ScriptEngine::FunctionSignature getter,
                                        ScriptEngine::FunctionSignature setter, const QString& parent) {
    if (QThread::currentThread() != QScriptEngine::thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngineQtScript::registerGetterSetter() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] "
            " name:" << name << "parent:" << parent;
#endif
        QMetaObject::invokeMethod(this, "registerGetterSetter",
                                  Q_ARG(const QString&, name),
                                  Q_ARG(ScriptEngine::FunctionSignature, getter),
                                  Q_ARG(ScriptEngine::FunctionSignature, setter),
                                  Q_ARG(const QString&, parent));
        return;
    }
#ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptEngineQtScript::registerGetterSetter() called on thread [" << QThread::currentThread() << "] name:" << name << "parent:" << parent;
#endif

    auto setterFunction = newFunction(setter, 1);
    auto getterFunction = newFunction(getter);

    if (!parent.isNull() && !parent.isEmpty()) {
        auto object = globalObject().property(parent);
        if (object.isValid()) {
            object.setProperty(name, setterFunction, ScriptValue::PropertySetter);
            object.setProperty(name, getterFunction, ScriptValue::PropertyGetter);
        }
    } else {
        globalObject().setProperty(name, setterFunction, ScriptValue::PropertySetter);
        globalObject().setProperty(name, getterFunction, ScriptValue::PropertyGetter);
    }
}

ScriptValue ScriptEngineQtScript::evaluateInClosure(const ScriptValue& _closure,
                                                           const ScriptProgramPointer& _program) {
    PROFILE_RANGE(script, "evaluateInClosure");
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return nullValue();
    }
    ScriptProgramQtWrapper* unwrappedProgram = ScriptProgramQtWrapper::unwrap(_program);
    if (unwrappedProgram == nullptr) {
        return nullValue();
    }
    const QScriptProgram& program = unwrappedProgram->toQtValue();

    const auto fileName = program.fileName();
    const auto shortName = QUrl(fileName).fileName();

    ScriptValueQtWrapper* unwrappedClosure = ScriptValueQtWrapper::unwrap(_closure);
    if (unwrappedClosure == nullptr) {
        return nullValue();
    }
    const QScriptValue& closure = unwrappedClosure->toQtValue();

    QScriptValue oldGlobal;
    auto global = closure.property("global");
    if (global.isObject()) {
#ifdef DEBUG_JS
        qCDebug(shared) << " setting global = closure.global" << shortName;
#endif
        oldGlobal = QScriptEngine::globalObject();
        setGlobalObject(global);
    }

    auto context = pushContext();

    auto thiz = closure.property("this");
    if (thiz.isObject()) {
#ifdef DEBUG_JS
        qCDebug(shared) << " setting this = closure.this" << shortName;
#endif
        context->setThisObject(thiz);
    }

    context->pushScope(closure);
#ifdef DEBUG_JS
    qCDebug(shared) << QString("[%1] evaluateInClosure %2").arg(isEvaluating()).arg(shortName);
#endif
    ScriptValue result;
    {
        auto qResult = QScriptEngine::evaluate(program);

        if (hasUncaughtException()) {
            auto err = cloneUncaughtException(__FUNCTION__);
#ifdef DEBUG_JS_EXCEPTIONS
            qCWarning(shared) << __FUNCTION__ << "---------- hasCaught:" << err.toString() << result.toString();
            err.setProperty("_result", result);
#endif
            result = err;
        } else {
            result = ScriptValue(new ScriptValueQtWrapper(this, std::move(qResult)));
        }
    }
#ifdef DEBUG_JS
    qCDebug(shared) << QString("[%1] //evaluateInClosure %2").arg(isEvaluating()).arg(shortName);
#endif
    popContext();

    if (oldGlobal.isValid()) {
#ifdef DEBUG_JS
        qCDebug(shared) << " restoring global" << shortName;
#endif
        setGlobalObject(oldGlobal);
    }

    return result;
}

ScriptValue ScriptEngineQtScript::evaluate(const QString& sourceCode, const QString& fileName) {
    if (_scriptManager && _scriptManager->isStopped()) {
        return undefinedValue(); // bail early
    }

    if (QThread::currentThread() != QScriptEngine::thread()) {
        ScriptValue result;
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngineQtScript::evaluate() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] "
            "sourceCode:" << sourceCode << " fileName:" << fileName;
#endif
        BLOCKING_INVOKE_METHOD(this, "evaluate",
                                  Q_RETURN_ARG(ScriptValue, result),
                                  Q_ARG(const QString&, sourceCode),
                                  Q_ARG(const QString&, fileName));
        return result;
    }

    // Check syntax
    auto syntaxError = lintScript(sourceCode, fileName);
    if (syntaxError.isError()) {
        if (!isEvaluating()) {
            syntaxError.setProperty("detail", "evaluate");
        }
        raiseException(syntaxError);
        maybeEmitUncaughtException("lint");
        return syntaxError;
    }
    QScriptProgram program { sourceCode, fileName, 1 };
    if (program.isNull()) {
        // can this happen?
        auto err = makeError(newValue("could not create QScriptProgram for " + fileName));
        raiseException(err);
        maybeEmitUncaughtException("compile");
        return err;
    }

    QScriptValue result = QScriptEngine::evaluate(program);
    maybeEmitUncaughtException("evaluate");
    
    return ScriptValue(new ScriptValueQtWrapper(this, std::move(result)));
}

Q_INVOKABLE ScriptValue ScriptEngineQtScript::evaluate(const ScriptProgramPointer& program) {
    if (_scriptManager && _scriptManager->isStopped()) {
        return undefinedValue(); // bail early
    }

    if (QThread::currentThread() != QScriptEngine::thread()) {
        ScriptValue result;
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngineQtScript::evaluate() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] "
            "sourceCode:" << sourceCode << " fileName:" << fileName;
#endif
        BLOCKING_INVOKE_METHOD(this, "evaluate",
                                  Q_RETURN_ARG(ScriptValue, result),
                                  Q_ARG(const ScriptProgramPointer&, program));
        return result;
    }

    ScriptProgramQtWrapper* unwrapped = ScriptProgramQtWrapper::unwrap(program);
    if (!unwrapped) {
        auto err = makeError(newValue("could not unwrap program"));
        raiseException(err);
        maybeEmitUncaughtException("compile");
        return err;
    }

    const QScriptProgram& qProgram = unwrapped->toQtValue();
    if (qProgram.isNull()) {
        // can this happen?
        auto err = makeError(newValue("requested program is empty"));
        raiseException(err);
        maybeEmitUncaughtException("compile");
        return err;
    }

    QScriptValue result = QScriptEngine::evaluate(qProgram);
    maybeEmitUncaughtException("evaluate");

    return ScriptValue(new ScriptValueQtWrapper(this, std::move(result)));
}


void ScriptEngineQtScript::updateMemoryCost(const qint64& deltaSize) {
    if (deltaSize > 0) {
        // We've patched qt to fix https://highfidelity.atlassian.net/browse/BUGZ-46 on mac and windows only.
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
        reportAdditionalMemoryCost(deltaSize);
#endif
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ScriptEngine implementation

ScriptValue ScriptEngineQtScript::globalObject() const {
    QScriptValue global = QScriptEngine::globalObject(); // can't cache the value as it may change
    return ScriptValue(new ScriptValueQtWrapper(const_cast<ScriptEngineQtScript*>(this), std::move(global)));
}

ScriptManager* ScriptEngineQtScript::manager() const {
    return _scriptManager;
}

ScriptValue ScriptEngineQtScript::newArray(uint length) {
    QScriptValue result = QScriptEngine::newArray(length);
    return ScriptValue(new ScriptValueQtWrapper(this, std::move(result)));
}

ScriptValue ScriptEngineQtScript::newArrayBuffer(const QByteArray& message) {
    QScriptValue data = QScriptEngine::newVariant(QVariant::fromValue(message));
    QScriptValue ctor = QScriptEngine::globalObject().property("ArrayBuffer");
    auto array = qscriptvalue_cast<ArrayBufferClass*>(ctor.data());
    if (!array) {
        return undefinedValue();
    }
    QScriptValue result = QScriptEngine::newObject(array, data);
    return ScriptValue(new ScriptValueQtWrapper(this, std::move(result)));
}

ScriptValue ScriptEngineQtScript::newObject() {
    QScriptValue result = QScriptEngine::newObject();
    return ScriptValue(new ScriptValueQtWrapper(this, std::move(result)));
}

ScriptProgramPointer ScriptEngineQtScript::newProgram(const QString& sourceCode, const QString& fileName) {
    QScriptProgram result(sourceCode, fileName);
    return std::make_shared<ScriptProgramQtWrapper>(this, result);
}

ScriptValue ScriptEngineQtScript::newQObject(QObject* object,
                                                    ScriptEngine::ValueOwnership ownership,
                                                    const ScriptEngine::QObjectWrapOptions& options) {
    QScriptValue result = ScriptObjectQtProxy::newQObject(this, object, ownership, options);
    return ScriptValue(new ScriptValueQtWrapper(this, std::move(result)));
}

ScriptValue ScriptEngineQtScript::newValue(bool value) {
    QScriptValue result(this, value);
    return ScriptValue(new ScriptValueQtWrapper(this, std::move(result)));
}

ScriptValue ScriptEngineQtScript::newValue(int value) {
    QScriptValue result(this, value);
    return ScriptValue(new ScriptValueQtWrapper(this, std::move(result)));
}

ScriptValue ScriptEngineQtScript::newValue(uint value) {
    QScriptValue result(this, value);
    return ScriptValue(new ScriptValueQtWrapper(this, std::move(result)));
}

ScriptValue ScriptEngineQtScript::newValue(double value) {
    QScriptValue result(this, value);
    return ScriptValue(new ScriptValueQtWrapper(this, std::move(result)));
}

ScriptValue ScriptEngineQtScript::newValue(const QString& value) {
    QScriptValue result(this, value);
    return ScriptValue(new ScriptValueQtWrapper(this, std::move(result)));
}

ScriptValue ScriptEngineQtScript::newValue(const QLatin1String& value) {
    QScriptValue result(this, value);
    return ScriptValue(new ScriptValueQtWrapper(this, std::move(result)));
}

ScriptValue ScriptEngineQtScript::newValue(const char* value) {
    QScriptValue result(this, value);
    return ScriptValue(new ScriptValueQtWrapper(this, std::move(result)));
}

ScriptValue ScriptEngineQtScript::newVariant(const QVariant& value) {
    QScriptValue result = castVariantToValue(value);
    return ScriptValue(new ScriptValueQtWrapper(this, std::move(result)));
}

ScriptValue ScriptEngineQtScript::nullValue() {
    return _nullValue;
}

ScriptValue ScriptEngineQtScript::undefinedValue() {
    return _undefinedValue;
}

void ScriptEngineQtScript::abortEvaluation() {
    QScriptEngine::abortEvaluation();
}

void ScriptEngineQtScript::clearExceptions() {
    QScriptEngine::clearExceptions();
}

ScriptContext* ScriptEngineQtScript::currentContext() const {
    QScriptContext* localCtx = QScriptEngine::currentContext();
    if (!localCtx) {
        return nullptr;
    }
    if (!_currContext || _currContext->toQtValue() != localCtx) {
        _currContext = std::make_shared<ScriptContextQtWrapper>(const_cast<ScriptEngineQtScript*>(this), localCtx);
    }
    return _currContext.get();
}

bool ScriptEngineQtScript::hasUncaughtException() const {
    return QScriptEngine::hasUncaughtException();
}

bool ScriptEngineQtScript::isEvaluating() const {
    return QScriptEngine::isEvaluating();
}

ScriptValue ScriptEngineQtScript::newFunction(ScriptEngine::FunctionSignature fun, int length) {
    auto innerFunc = [](QScriptContext* _context, QScriptEngine* _engine) -> QScriptValue {
        auto callee = _context->callee();
        QVariant funAddr = callee.property("_func").toVariant();
        ScriptEngine::FunctionSignature fun = reinterpret_cast<ScriptEngine::FunctionSignature>(funAddr.toULongLong());
        ScriptEngineQtScript* engine = static_cast<ScriptEngineQtScript*>(_engine);
        ScriptContextQtWrapper context(engine, _context);
        ScriptValue result = fun(&context, engine);
        ScriptValueQtWrapper* unwrapped = ScriptValueQtWrapper::unwrap(result);
        return unwrapped ? unwrapped->toQtValue() : QScriptValue();
    };

    QScriptValue result = QScriptEngine::newFunction(innerFunc, length);
    auto funAddr = QScriptEngine::newVariant(QVariant(reinterpret_cast<qulonglong>(fun)));
    result.setProperty("_func", funAddr, QScriptValue::PropertyFlags(QScriptValue::ReadOnly + QScriptValue::Undeletable + QScriptValue::SkipInEnumeration));
    return ScriptValue(new ScriptValueQtWrapper(this, std::move(result)));
}

void ScriptEngineQtScript::setObjectName(const QString& name) {
    QScriptEngine::setObjectName(name);
}

bool ScriptEngineQtScript::setProperty(const char* name, const QVariant& value) {
    return QScriptEngine::setProperty(name, value);
}

void ScriptEngineQtScript::setProcessEventsInterval(int interval) {
    QScriptEngine::setProcessEventsInterval(interval);
}

QThread* ScriptEngineQtScript::thread() const {
    return QScriptEngine::thread();
}

void ScriptEngineQtScript::setThread(QThread* thread) {
    moveToThread(thread);
}

ScriptValue ScriptEngineQtScript::uncaughtException() const {
    QScriptValue result = QScriptEngine::uncaughtException();
    return ScriptValue(new ScriptValueQtWrapper(const_cast<ScriptEngineQtScript*>(this), std::move(result)));
}

QStringList ScriptEngineQtScript::uncaughtExceptionBacktrace() const {
    return QScriptEngine::uncaughtExceptionBacktrace();
}

int ScriptEngineQtScript::uncaughtExceptionLineNumber() const {
    return QScriptEngine::uncaughtExceptionLineNumber();
}

bool ScriptEngineQtScript::raiseException(const ScriptValue& exception) {
    ScriptValueQtWrapper* unwrapped = ScriptValueQtWrapper::unwrap(exception);
    QScriptValue qException = unwrapped ? unwrapped->toQtValue() : QScriptEngine::newVariant(exception.toVariant());
    return raiseException(qException);
}

ScriptValue ScriptEngineQtScript::create(int type, const void* ptr) {
    QVariant variant(type, ptr);
    QScriptValue scriptValue = castVariantToValue(variant);
    return ScriptValue(new ScriptValueQtWrapper(this, std::move(scriptValue)));
}

QVariant ScriptEngineQtScript::convert(const ScriptValue& value, int typeId) {
    ScriptValueQtWrapper* unwrapped = ScriptValueQtWrapper::unwrap(value);
    if (unwrapped == nullptr) {
        return QVariant();
    }

    QVariant var;
    if (!castValueToVariant(unwrapped->toQtValue(), var, typeId)) {
        return QVariant();
    }

    int destType = var.userType();
    if (destType != typeId) {
        var.convert(typeId);  // if conversion fails then var is set to QVariant()
    }

    return var;
}
