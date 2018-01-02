#pragma once

// Minimalist threadsafe Promise-like helper for managing asynchronous results
//
// This class pivots around continuation-style callback handlers:
//     auto successCallback = [=](QVariantMap result) { .... }
//     auto errorCallback = [=](QString error) { .... }
//     auto combinedCallback = [=](QString error, QVariantMap result) { .... }
//
// * Callback Handlers are automatically invoked on the right thread (the Promise's thread).
// * Callbacks can be assigned anytime during a Promise's life and "do the right thing".
//   - ie: for code clarity you can define success cases first or choose to maintain time order
// * "Deferred" concept can be used to publish outcomes.
// * "Promise" concept be used to subscribe to outcomes.
//
// See AssetScriptingInterface.cpp for some examples of using to simplify chained async results.

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QVariantMap>
#include <QDebug>
#include "ReadWriteLockable.h"
#include <memory>

class MiniPromise : public QObject, public std::enable_shared_from_this<MiniPromise>, public ReadWriteLockable {
    Q_OBJECT
public:
    using HandlerFunction = std::function<void(QString error, QVariantMap result)>;
    using SuccessFunction = std::function<void(QVariantMap result)>;
    using ErrorFunction = std::function<void(QString error)>;
    using HandlerFunctions = QVector<HandlerFunction>;
    using Promise = std::shared_ptr<MiniPromise>;

    static int metaTypeID;

    MiniPromise() {}
    MiniPromise(const QString debugName) { setObjectName(debugName); }

    ~MiniPromise() {
        if (!_rejected && !_resolved) {
            qWarning() << "MiniPromise::~MiniPromise -- destroying unhandled promise:" << objectName() << _error << _result;
        }
    }
    Promise self() { return shared_from_this(); }

    Q_INVOKABLE void executeOnPromiseThread(std::function<void()> function) {
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(
                this, "executeOnPromiseThread", Qt::BlockingQueuedConnection,
                Q_ARG(std::function<void()>, function));
        } else {
            function();
        }
    }

    // result aggregation helpers -- eg: deferred->defaults(interimResultMap)->ready(...)
    // copy values from the input map, but only for keys that don't already exist
    Promise mixin(const QVariantMap& source) {
        withWriteLock([&]{
            for (const auto& key : source.keys()) {
                if (!_result.contains(key)) {
                    _result[key] = source[key];
                }
            }
        });
        return self();
    }
    // copy values from the input map, replacing any existing keys
    Promise assignResult(const QVariantMap& source) {
        withWriteLock([&]{
            for (const auto& key : source.keys()) {
                _result[key] = source[key];
            }
        });
        return self();
    }

    // callback registration methods
    Promise ready(HandlerFunction always) { return finally(always); }
    Promise finally(HandlerFunction always) {
        if (!_rejected && !_resolved) {
            withWriteLock([&]{
                _onfinally << always;
            });
        } else {
            qWarning() << "MiniPromise::finally() called after promise was already rejected or resolved:" << objectName();
            executeOnPromiseThread([&]{
                withReadLock([&]{
                    always(_error, _result);
                });
            });
        }
        return self();
    }
    Promise fail(ErrorFunction errorOnly) {
        return fail([this, errorOnly](QString error, QVariantMap result) {
            errorOnly(error);
        });
    }

    Promise fail(HandlerFunction failFunc) {
        if (!_rejected) {
            withWriteLock([&]{
                _onreject << failFunc;
            });
        } else {
            executeOnPromiseThread([&]{
                qWarning() << "MiniPromise::fail() called after promise was already rejected:" << objectName();
                withReadLock([&]{
                    failFunc(_error, _result);
                });
            });
        }
        return self();
    }

    Promise then(SuccessFunction successOnly) {
        return then([this, successOnly](QString error, QVariantMap result) {
            successOnly(result);
        });
    }
    Promise then(HandlerFunction successFunc) {
        if (!_resolved) {
            withWriteLock([&]{
                _onresolve << successFunc;
            });
        } else {
            executeOnPromiseThread([&]{
                qWarning() << "MiniPromise::then() called after promise was already resolved:" << objectName();
                withReadLock([&]{
                    successFunc(_error, _result);
                });
            });
        }
        return self();
    }

    // NOTE: first arg may be null (see ES6 .then(null, errorHandler) conventions)
    Promise then(SuccessFunction successOnly, ErrorFunction errorOnly) {
        if (successOnly) {
            then(successOnly);
        }
        if (errorOnly) {
            fail(errorOnly);
        }
        return self();
    }

    // trigger methods
    // handle() automatically resolves or rejects the promise (based on whether an error value occurred)
    Promise handle(QString error, const QVariantMap& result) {
        if (error.isEmpty()) {
            resolve(error, result);
        } else {
            reject(error, result);
        }
        return self();
    }

    Promise resolve(QVariantMap result) {
        return resolve(QString(), result);
    }
    Promise resolve(QString error, const QVariantMap& result) {
        setState(true, error, result);

        QString localError;
        QVariantMap localResult;
        HandlerFunctions resolveHandlers;
        HandlerFunctions finallyHandlers;
        withReadLock([&]{
            localError = _error;
            localResult = _result;
            resolveHandlers = _onresolve;
            finallyHandlers = _onfinally;
        });
        executeOnPromiseThread([&]{
            for (const auto& onresolve : resolveHandlers) {
                onresolve(localError, localResult);
            }
            for (const auto& onfinally : finallyHandlers) {
                onfinally(localError, localResult);
            }
        });
        return self();
    }

    Promise reject(QString error) {
        return reject(error, QVariantMap());
    }
    Promise reject(QString error, const QVariantMap& result) {
        setState(false, error, result);

        QString localError;
        QVariantMap localResult;
        HandlerFunctions rejectHandlers;
        HandlerFunctions finallyHandlers;
        withReadLock([&]{
            localError = _error;
            localResult = _result;
            rejectHandlers = _onreject;
            finallyHandlers = _onfinally;
        });
        executeOnPromiseThread([&]{
            for (const auto& onreject : rejectHandlers) {
                onreject(localError, localResult);
            }
            for (const auto& onfinally : finallyHandlers) {
                onfinally(localError, localResult);
            }
        });
        return self();
    }

private:

    Promise setState(bool resolved, QString error, const QVariantMap& result) {
        if (resolved) {
            _resolved = true;
        } else {
            _rejected = true;
        }
        withWriteLock([&]{
            _error = error;
        });
        assignResult(result);
        return self();
    }

    QString _error;
    QVariantMap _result;
    std::atomic<bool> _rejected{false};
    std::atomic<bool> _resolved{false};
    HandlerFunctions _onresolve;
    HandlerFunctions _onreject;
    HandlerFunctions _onfinally;
};

inline MiniPromise::Promise makePromise(const QString& hint = QString()) {
    return std::make_shared<MiniPromise>(hint);
}

Q_DECLARE_METATYPE(MiniPromise::Promise)
