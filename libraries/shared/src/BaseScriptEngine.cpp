//
//  BaseScriptEngine.cpp
//  libraries/script-engine/src
//
//  Created by Timothy Dedischew on 02/01/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BaseScriptEngine.h"
#include "SharedLogging.h"

#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QUrl>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptValueIterator>
#include <QtScript/QScriptContextInfo>

#include "Profile.h"

const QString BaseScriptEngine::SCRIPT_EXCEPTION_FORMAT { "[%0] %1 in %2:%3" };
const QString BaseScriptEngine::SCRIPT_BACKTRACE_SEP { "\n    " };

bool BaseScriptEngine::IS_THREADSAFE_INVOCATION(const QThread *thread, const QString& method) {
    if (QThread::currentThread() == thread) {
        return true;
    }
    qCCritical(shared) << QString("Scripting::%1 @ %2 -- ignoring thread-unsafe call from %3")
        .arg(method).arg(thread ? thread->objectName() : "(!thread)").arg(QThread::currentThread()->objectName());
    qCDebug(shared) << "(please resolve on the calling side by using invokeMethod, executeOnScriptThread, etc.)";
    Q_ASSERT(false);
    return false;
}

// engine-aware JS Error copier and factory
QScriptValue BaseScriptEngine::makeError(const QScriptValue& _other, const QString& type) {
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return unboundNullValue();
    }
    auto other = _other;
    if (other.isString()) {
        other = newObject();
        other.setProperty("message", _other.toString());
    }
    auto proto = globalObject().property(type);
    if (!proto.isFunction()) {
        proto = globalObject().property(other.prototype().property("constructor").property("name").toString());
    }
    if (!proto.isFunction()) {
#ifdef DEBUG_JS_EXCEPTIONS
        qCDebug(shared) << "BaseScriptEngine::makeError -- couldn't find constructor for" << type << " -- using Error instead";
#endif
        proto = globalObject().property("Error");
    }
    if (other.engine() != this) {
        // JS Objects are parented to a specific script engine instance
        // -- this effectively ~clones it locally by routing through a QVariant and back
        other = toScriptValue(other.toVariant());
    }
    // ~ var err = new Error(other.message)
    auto err = proto.construct(QScriptValueList({other.property("message")}));

    // transfer over any existing properties
    QScriptValueIterator it(other);
    while (it.hasNext()) {
        it.next();
        err.setProperty(it.name(), it.value());
    }
    return err;
}

// check syntax and when there are issues returns an actual "SyntaxError" with the details
QScriptValue BaseScriptEngine::lintScript(const QString& sourceCode, const QString& fileName, const int lineNumber) {
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return unboundNullValue();
    }
    const auto syntaxCheck = checkSyntax(sourceCode);
    if (syntaxCheck.state() != QScriptSyntaxCheckResult::Valid) {
        auto err = globalObject().property("SyntaxError")
            .construct(QScriptValueList({syntaxCheck.errorMessage()}));
        err.setProperty("fileName", fileName);
        err.setProperty("lineNumber", syntaxCheck.errorLineNumber());
        err.setProperty("expressionBeginOffset", syntaxCheck.errorColumnNumber());
        err.setProperty("stack", currentContext()->backtrace().join(SCRIPT_BACKTRACE_SEP));
        {
            const auto error = syntaxCheck.errorMessage();
            const auto line = QString::number(syntaxCheck.errorLineNumber());
            const auto column = QString::number(syntaxCheck.errorColumnNumber());
            // for compatibility with legacy reporting
            const auto message = QString("[SyntaxError] %1 in %2:%3(%4)").arg(error, fileName, line, column);
            err.setProperty("formatted", message);
        }
        return err;
    }
    return QScriptValue();
}

// this pulls from the best available information to create a detailed snapshot of the current exception
QScriptValue BaseScriptEngine::cloneUncaughtException(const QString& extraDetail) {
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return unboundNullValue();
    }
    if (!hasUncaughtException()) {
        return unboundNullValue();
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
        backtrace = exception.property("stack").toString().split(SCRIPT_BACKTRACE_SEP);
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
        for (auto c = currentContext(); c && fileName.isEmpty(); c = c->parentContext()) {
            QScriptContextInfo info { c };
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
    err.setProperty("lineNumber", lineNumber );
    err.setProperty("detail", detail);
    err.setProperty("stack", backtrace.join(SCRIPT_BACKTRACE_SEP));

#ifdef DEBUG_JS_EXCEPTIONS
    err.setProperty("_fileName", exception.property("fileName").toString());
    err.setProperty("_stack", uncaughtExceptionBacktrace().join(SCRIPT_BACKTRACE_SEP));
    err.setProperty("_lineNumber", uncaughtExceptionLineNumber());
#endif
    return err;
}

QString BaseScriptEngine::formatException(const QScriptValue& exception, bool includeExtendedDetails) {
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return QString();
    }
    QString note { "UncaughtException" };
    QString result;

    if (!exception.isObject()) {
        return result;
    }
    const auto message = exception.toString();
    const auto fileName = exception.property("fileName").toString();
    const auto lineNumber = exception.property("lineNumber").toString();
    const auto stacktrace =  exception.property("stack").toString();

    if (includeExtendedDetails) {
        // Display additional exception / troubleshooting hints that can be added via the custom Error .detail property
        // Example difference:
        //   [UncaughtExceptions] Error: Can't find variable: foobar in atp:/myentity.js\n...
        //   [UncaughtException (construct {1eb5d3fa-23b1-411c-af83-163af7220e3f})] Error: Can't find variable: foobar in atp:/myentity.js\n...
        if (exception.property("detail").isValid()) {
            note += " " + exception.property("detail").toString();
        }
    }

    result = QString(SCRIPT_EXCEPTION_FORMAT).arg(note, message, fileName, lineNumber);
    if (!stacktrace.isEmpty()) {
        result += QString("\n[Backtrace]%1%2").arg(SCRIPT_BACKTRACE_SEP).arg(stacktrace);
    }
    return result;
}

bool BaseScriptEngine::raiseException(const QScriptValue& exception) {
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return false;
    }
    if (currentContext()) {
        // we have an active context / JS stack frame so throw the exception per usual
        currentContext()->throwValue(makeError(exception));
        return true;
    } else {
        // we are within a pure C++ stack frame (ie: being called directly by other C++ code)
        // in this case no context information is available so just emit the exception for reporting
        emit unhandledException(makeError(exception));
    }
    return false;
}

bool BaseScriptEngine::maybeEmitUncaughtException(const QString& debugHint) {
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return false;
    }
    if (!isEvaluating() && hasUncaughtException()) {
        emit unhandledException(cloneUncaughtException(debugHint));
        clearExceptions();
        return true;
    }
    return false;
}

QScriptValue BaseScriptEngine::evaluateInClosure(const QScriptValue& closure, const QScriptProgram& program) {
    PROFILE_RANGE(script, "evaluateInClosure");
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return unboundNullValue();
    }
    const auto fileName = program.fileName();
    const auto shortName = QUrl(fileName).fileName();

    QScriptValue result;
    QScriptValue oldGlobal;
    auto global = closure.property("global");
    if (global.isObject()) {
#ifdef DEBUG_JS
        qCDebug(shared) << " setting global = closure.global" << shortName;
#endif
        oldGlobal = globalObject();
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
    {
        result = BaseScriptEngine::evaluate(program);
        if (hasUncaughtException()) {
            auto err = cloneUncaughtException(__FUNCTION__);
#ifdef DEBUG_JS_EXCEPTIONS
            qCWarning(shared) << __FUNCTION__ << "---------- hasCaught:" << err.toString() << result.toString();
            err.setProperty("_result", result);
#endif
            result = err;
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

// Lambda
QScriptValue BaseScriptEngine::newLambdaFunction(std::function<QScriptValue(QScriptContext *, QScriptEngine*)> operation, const QScriptValue& data, const QScriptEngine::ValueOwnership& ownership) {
    auto lambda = new Lambda(this, operation, data);
    auto object = newQObject(lambda, ownership);
    auto call = object.property("call");
    call.setPrototype(object); // context->callee().prototype() === Lambda QObject
    call.setData(data);        // context->callee().data() will === data param
    return call;
}
QString Lambda::toString() const {
    return QString("[Lambda%1]").arg(data.isValid() ? " " + data.toString() : data.toString());
}

Lambda::~Lambda() {
#ifdef DEBUG_JS_LAMBDA_FUNCS
    qDebug() << "~Lambda" << "this" << this;
#endif
}

Lambda::Lambda(QScriptEngine *engine, std::function<QScriptValue(QScriptContext *, QScriptEngine*)> operation, QScriptValue data)
    : engine(engine), operation(operation), data(data) {
#ifdef DEBUG_JS_LAMBDA_FUNCS
    qDebug() << "Lambda" << data.toString();
#endif
}
QScriptValue Lambda::call() {
    if (!BaseScriptEngine::IS_THREADSAFE_INVOCATION(engine->thread(), __FUNCTION__)) {
        return BaseScriptEngine::unboundNullValue();
    }
    return operation(engine->currentContext(), engine);
}

QScriptValue makeScopedHandlerObject(QScriptValue scopeOrCallback, QScriptValue methodOrName) {
    auto engine = scopeOrCallback.engine();
    if (!engine) {
        return scopeOrCallback;
    }
    auto scope = QScriptValue();
    auto callback = scopeOrCallback;
    if (scopeOrCallback.isObject()) {
        if (methodOrName.isString()) {
            scope = scopeOrCallback;
            callback = scope.property(methodOrName.toString());
        } else if (methodOrName.isFunction()) {
            scope = scopeOrCallback;
            callback = methodOrName;
        }
    }
    auto handler = engine->newObject();
    handler.setProperty("scope", scope);
    handler.setProperty("callback", callback);
    return handler;
}

QScriptValue callScopedHandlerObject(QScriptValue handler, QScriptValue err, QScriptValue result) {
    return handler.property("callback").call(handler.property("scope"), QScriptValueList({ err, result }));
}

#ifdef DEBUG_JS
void BaseScriptEngine::_debugDump(const QString& header, const QScriptValue& object, const QString& footer) {
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
