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


namespace hifi { namespace qt {
void addBlockingForbiddenThread(const QString& name, QThread* thread = nullptr);

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

} }

#define BLOCKING_INVOKE_METHOD(obj, member, ...) \
    hifi::qt::blockingInvokeMethod(__FUNCTION__, obj, member, ##__VA_ARGS__)

#endif
