//
//  Created by Bradley Austin Davis on 2015/11/09
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QtHelpers.h"

#include <QtCore/QThread>
#include <QtCore/QCoreApplication>
#include <QtCore/QLoggingCategory>
#include <QtCore/QReadWriteLock>

// Inspecting of the qt event queue depth
// requres access to private Qt datastructures
// defined in private Qt headers
#ifdef DEBUG_EVENT_QUEUE
#include "../TryLocker.h"
#define QT_BOOTSTRAPPED
#include <private/qthread_p.h>
#include <private/qobject_p.h>
#undef QT_BOOTSTRAPPED
#endif // DEBUG_EVENT_QUEUE

#include "../Profile.h"
Q_LOGGING_CATEGORY(thread_safety, "hifi.thread_safety")

namespace hifi { namespace qt {

static QHash<QThread*, QString> threadHash;
static QReadWriteLock threadHashLock;

void addBlockingForbiddenThread(const QString& name, QThread* thread) {
    if (!thread) {
        thread = QThread::currentThread();
    }
    QWriteLocker locker(&threadHashLock);
    threadHash[thread] = name;
}

bool blockingInvokeMethod(
    const char* function,
    QObject *obj, const char *member,
    QGenericReturnArgument ret,
    QGenericArgument val0,
    QGenericArgument val1,
    QGenericArgument val2,
    QGenericArgument val3,
    QGenericArgument val4,
    QGenericArgument val5,
    QGenericArgument val6,
    QGenericArgument val7,
    QGenericArgument val8,
    QGenericArgument val9) {
    auto currentThread = QThread::currentThread();
    if (currentThread == qApp->thread()) {
        qCWarning(thread_safety) << "BlockingQueuedConnection invoked on main thread from " << function;
        return QMetaObject::invokeMethod(obj, member,
            Qt::BlockingQueuedConnection, ret, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
    } 

    {
        QReadLocker locker(&threadHashLock);
        for (const auto& thread : threadHash.keys()) {
            if (currentThread == thread) {
                qCWarning(thread_safety) << "BlockingQueuedConnection invoked on forbidden thread " << threadHash[thread];
            }
        }
    }

    PROFILE_RANGE(app, function);
    return QMetaObject::invokeMethod(obj, member,
            Qt::BlockingQueuedConnection, ret, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
}

bool blockingInvokeMethod(
    const char* function,
    QObject *obj, const char *member,
    QGenericArgument val0,
    QGenericArgument val1,
    QGenericArgument val2,
    QGenericArgument val3,
    QGenericArgument val4,
    QGenericArgument val5,
    QGenericArgument val6,
    QGenericArgument val7,
    QGenericArgument val8,
    QGenericArgument val9) {
    return blockingInvokeMethod(function, obj, member, QGenericReturnArgument(), val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
}

// Inspecting of the qt event queue
// requres access to private Qt datastructures
// Querying the event queue should be done with
// care as it could lock the threadData->postEventList.mutex
// The code uses a tryLock to avoid the possability of a
// deadlock during a call to this code, although that is unlikely
//
#ifdef DEBUG_EVENT_QUEUE
int getEventQueueSize(QThread* thread) {
    auto threadData = QThreadData::get2(thread);
    {
        MutexTryLocker locker(threadData->postEventList.mutex);
        if (locker.isLocked()) {
            return threadData->postEventList.size();
        }
    }
    return -1;
}

void dumpEventQueue(QThread* thread) {
    auto threadData = QThreadData::get2(thread);
    QMutexLocker locker(&threadData->postEventList.mutex);
    qDebug() << "Event list, size =" << threadData->postEventList.size();
    for (auto& postEvent : threadData->postEventList) {
        QEvent::Type type = (postEvent.event ? postEvent.event->type() : QEvent::None);
        qDebug() << "    " << type;
    }
}

#endif // DEBUG_EVENT_QUEUE

} }
