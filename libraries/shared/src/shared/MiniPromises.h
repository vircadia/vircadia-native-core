#pragma once

// Minimalist threadsafe Promise-like helper for instrumenting asynchronous results
//
// This class pivots around composable continuation-style callback handlers:
//     auto successCallback = [=](QVariantMap result) { .... }
//     auto errorCallback = [=](QString error) { .... }
//     auto combinedCallback = [=](QString error, QVariantMap result) { .... }
//
// * Callback Handlers are automatically invoked on the right thread (the Promise's thread).
// * Callbacks can be assigned anytime during a Promise's life and "do the right thing".
//   - ie: for code clarity you can define success cases first (or maintain time order)
// * "Deferred" concept can be used to publish outcomes.
// * "Promise" concept be used to subscribe to outcomes.
//
// See AssetScriptingInterface.cpp for some examples of using to simplify chained async result.

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QVariantMap>
#include <QDebug>
#include "ReadWriteLockable.h"
#include <memory>

class MiniPromise : public QObject, public std::enable_shared_from_this<MiniPromise>, public ReadWriteLockable {
    Q_OBJECT
public:
    using handlerFunction = std::function<void(QString error, QVariantMap result)>;
    using successFunction = std::function<void(QVariantMap result)>;
    using errorFunction = std::function<void(QString error)>;
    using handlers = QVector<handlerFunction>;
    using Promise = std::shared_ptr<MiniPromise>;
    MiniPromise(const QString debugName) { setObjectName(debugName); }
    MiniPromise() {}
    ~MiniPromise() {
        qDebug() << "~MiniPromise" << objectName();
        if (!_rejected && !_resolved) {
            qWarning() << "====== WARNING: unhandled MiniPromise" << objectName() << _error << _result;
        }
    }

    QString _error;
    QVariantMap _result;
    std::atomic<bool> _rejected{false};
    std::atomic<bool> _resolved{false};
    handlers _onresolve;
    handlers _onreject;
    handlers _onfinally;

    Promise self() { return shared_from_this(); }

    // result aggregation helpers -- eg: deferred->defaults(interimResultMap)->ready(...)

    // copy values from the input map, but only for keys that don't already exist
    Promise mixin(const QVariantMap& source) {
        qDebug() << objectName() << "mixin";
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
        qDebug() << objectName() << "assignResult";
        withWriteLock([&]{
            for (const auto& key : source.keys()) {
                _result[key] = source[key];
            }
        });
        return self();
    }

    // TODO: I think calling as "ready" makes it read better, but is customary Promise "finally" sufficient?
    Promise ready(handlerFunction always) { return finally(always); }
    Promise finally(handlerFunction always) {
        if (!_rejected && !_resolved) {
            withWriteLock([&]{
                _onfinally << always;
            });
        } else {
            qDebug() << "finally (already resolved/rejected)" << objectName();
            executeOnPromiseThread([&]{
                withReadLock([&]{
                    always(_error, _result);
                });
            });
        }
        return self();
    }
    Promise fail(errorFunction errorOnly) {
        return fail([this, errorOnly](QString error, QVariantMap result) {
            errorOnly(error);
        });
    }

    Promise fail(handlerFunction failFunc) {
        if (!_rejected) {
            withWriteLock([&]{
                _onreject << failFunc;
            });
        } else {
            executeOnPromiseThread([&]{
                qDebug() << "fail (already rejected)" << objectName();
                withReadLock([&]{
                    failFunc(_error, _result);
                });
            });
        }
        return self();
    }

    Promise then(successFunction successOnly) {
        return then([this, successOnly](QString error, QVariantMap result) {
            successOnly(result);
        });
    }
    Promise then(handlerFunction successFunc) {
        if (!_resolved) {
            withWriteLock([&]{
                _onresolve << successFunc;
            });
        } else {
            executeOnPromiseThread([&]{
                qDebug() << "fail (already resolved)" << objectName();
                withReadLock([&]{
                    successFunc(_error, _result);
                });
            });
        }
        return self();
    }
    // register combined success/error handlers
    Promise then(successFunction successOnly, errorFunction errorOnly) {
        // note: first arg can be null (see ES6 .then(null, errorHandler) conventions)
        if (successOnly) {
            then(successOnly);
        }
        if (errorOnly) {
            fail(errorOnly);
        }
        return self();
    }

    // trigger methods
    Promise handle(QString error, const QVariantMap& result) {
        qDebug() << "handle" << objectName() << error;
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

    Q_INVOKABLE void executeOnPromiseThread(std::function<void()> function) {
        if (QThread::currentThread() != thread()) {
            qDebug() << "-0-0-00-0--0" << objectName() << "executeOnPromiseThread -- wrong thread" << QThread::currentThread();
            QMetaObject::invokeMethod(
                this, "executeOnPromiseThread", Qt::BlockingQueuedConnection,
                Q_ARG(std::function<void()>, function));
        } else {
            function();
        }
    }

    Promise setState(bool resolved, QString error, const QVariantMap& result) {
        qDebug() << "setState" << objectName() << resolved << error;
        if (resolved) {
            _resolved = true;
        } else {
            _rejected = true;
        }
        withWriteLock([&]{
            _error = error;
        });
        assignResult(result);
        qDebug() << "//setState" << objectName() << resolved << error;
        return self();
    }
    Promise resolve(QString error, const QVariantMap& result) {
        setState(true, error, result);
        qDebug() << "handle" << objectName() << error;
        {
            QString error;
            QVariantMap result;
            handlers toresolve;
            handlers tofinally;
            withReadLock([&]{
                error = _error;
                result = _result;
                toresolve = _onresolve;
                tofinally = _onfinally;
            });
            executeOnPromiseThread([&]{
                for (const auto& onresolve : toresolve) {
                    onresolve(error, result);
                }
                for (const auto& onfinally : tofinally) {
                    onfinally(error, result);
                }
            });
        }
        return self();
    }
    Promise reject(QString error) {
        return reject(error, QVariantMap());
    }
    Promise reject(QString error, const QVariantMap& result) {
        setState(false, error, result);
        qDebug() << "handle" << objectName() << error;
        {
            QString error;
            QVariantMap result;
            handlers toreject;
            handlers tofinally;
            withReadLock([&]{
                error = _error;
                result = _result;
                toreject = _onreject;
                tofinally = _onfinally;
            });
            executeOnPromiseThread([&]{
                for (const auto& onreject : toreject) {
                    onreject(error, result);
                }
                for (const auto& onfinally : tofinally) {
                    onfinally(error, result);
                }
                if (toreject.isEmpty() && tofinally.isEmpty()) {
                    qWarning() << "WARNING: unhandled MiniPromise::reject" << objectName() << error << result;
                }
            });
        }
        return self();
    }
};

inline MiniPromise::Promise makePromise(const QString& hint = QString()) {
    return std::make_shared<MiniPromise>(hint);
}

Q_DECLARE_METATYPE(MiniPromise::Promise)
