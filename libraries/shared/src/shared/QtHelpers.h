//
//  Created by Bradley Austin Davis on 2017/06/29
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Shared_QtHelpers_h
#define hifi_Shared_QtHelpers_h

#include <QtCore/QObject>
#include <QtCore/QLoggingCategory>
#include <QtCore/QThread>

#include "../Profile.h"

#if defined(Q_OS_WIN)
// Enable event queue debugging
#define DEBUG_EVENT_QUEUE
#endif

class QLoggingCategory;
const QLoggingCategory& thread_safety();

namespace hifi { namespace qt {
void addBlockingForbiddenThread(const QString& name, QThread* thread = nullptr);
QString isBlockingForbiddenThread(QThread* currentThread);

bool blockingInvokeMethod(
    const char* function,
    QObject *obj, const char *member,
    QGenericReturnArgument ret,
    QGenericArgument val0 = QGenericArgument(Q_NULLPTR),
    QGenericArgument val1 = QGenericArgument(),
    QGenericArgument val2 = QGenericArgument(),
    QGenericArgument val3 = QGenericArgument(),
    QGenericArgument val4 = QGenericArgument(),
    QGenericArgument val5 = QGenericArgument(),
    QGenericArgument val6 = QGenericArgument(),
    QGenericArgument val7 = QGenericArgument(),
    QGenericArgument val8 = QGenericArgument(),
    QGenericArgument val9 = QGenericArgument());

bool blockingInvokeMethod(
    const char* function,
    QObject *obj, const char *member,
    QGenericArgument val0 = QGenericArgument(Q_NULLPTR),
    QGenericArgument val1 = QGenericArgument(),
    QGenericArgument val2 = QGenericArgument(),
    QGenericArgument val3 = QGenericArgument(),
    QGenericArgument val4 = QGenericArgument(),
    QGenericArgument val5 = QGenericArgument(),
    QGenericArgument val6 = QGenericArgument(),
    QGenericArgument val7 = QGenericArgument(),
    QGenericArgument val8 = QGenericArgument(),
    QGenericArgument val9 = QGenericArgument());

// handling unregistered functions
template <typename Func, typename ReturnType>
typename std::enable_if<!std::is_convertible<Func, const char*>::value, bool>::type
blockingInvokeMethod(const char* callingFunction, QObject* context, Func function, ReturnType* retVal) {
    auto currentThread = QThread::currentThread();
    if (currentThread == qApp->thread()) {
        qCWarning(thread_safety) << "BlockingQueuedConnection invoked on main thread from " << callingFunction;
        return QMetaObject::invokeMethod(context, function, Qt::BlockingQueuedConnection, retVal);
    }

    QString forbiddenThread = isBlockingForbiddenThread(currentThread);
    if (!forbiddenThread.isEmpty()) {
        qCWarning(thread_safety) << "BlockingQueuedConnection invoked on forbidden thread " << forbiddenThread;
    }

    PROFILE_RANGE(app, callingFunction);
    return QMetaObject::invokeMethod(context, function, Qt::BlockingQueuedConnection, retVal);
}

template <typename Func>
typename std::enable_if<!std::is_convertible<Func, const char*>::value, bool>::type
blockingInvokeMethod(const char* callingFunction, QObject* context, Func function) {
    auto currentThread = QThread::currentThread();
    if (currentThread == qApp->thread()) {
        qCWarning(thread_safety) << "BlockingQueuedConnection invoked on main thread from " << callingFunction;
        return QMetaObject::invokeMethod(context, function, Qt::BlockingQueuedConnection);
    }

    QString forbiddenThread = isBlockingForbiddenThread(currentThread);
    if (!forbiddenThread.isEmpty()) {
        qCWarning(thread_safety) << "BlockingQueuedConnection invoked on forbidden thread " << forbiddenThread;
    }

    PROFILE_RANGE(app, callingFunction);
    return QMetaObject::invokeMethod(context, function, Qt::BlockingQueuedConnection);
}

// Inspecting of the qt event queue
// requres access to private Qt datastructures
// Querying the event queue should be done with
// care as it could lock the threadData->postEventList.mutex
// The code uses a tryLock to avoid the possability of a
// deadlock during a call to this code, although that is unlikely
// When getEventQueueSize fails to get the lock, it returns -1
#ifdef DEBUG_EVENT_QUEUE
    int getEventQueueSize(QThread* thread);
    void dumpEventQueue(QThread* thread);
#endif // DEBUG_EVENT_QUEUE

} }

#define BLOCKING_INVOKE_METHOD(obj, member, ...) \
    ::hifi::qt::blockingInvokeMethod(__FUNCTION__, obj, member, ##__VA_ARGS__)

#endif
