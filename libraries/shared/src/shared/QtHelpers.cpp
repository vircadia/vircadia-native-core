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

Q_LOGGING_CATEGORY(thread_safety, "hifi.thread_safety")

namespace hifi { namespace qt {

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
    if (QThread::currentThread() == qApp->thread()) {
        qCWarning(thread_safety) << "BlockingQueuedConnection invoked on main thread from " << function;
    }
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



} }
