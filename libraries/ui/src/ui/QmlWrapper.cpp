//
//  Created by Bradley Austin Davis on 2017/06/22
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QmlWrapper.h"

#include <QtCore/QThread>
#include <QtCore/QCoreApplication>

#include <shared/QtHelpers.h>

QmlWrapper::QmlWrapper(QObject* qmlObject, QObject* parent)
    : QObject(parent), _qmlObject(qmlObject) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
}

void QmlWrapper::writeProperty(QString propertyName, QVariant propertyValue) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "writeProperty", Q_ARG(QString, propertyName), Q_ARG(QVariant, propertyValue));
    }
    _qmlObject->setProperty(propertyName.toStdString().c_str(), propertyValue);
}

void QmlWrapper::writeProperties(QVariant propertyMap) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "writeProperties", Q_ARG(QVariant, propertyMap));
    }
    QVariantMap map = propertyMap.toMap();
    for (const QString& key : map.keys()) {
        _qmlObject->setProperty(key.toStdString().c_str(), map[key]);
    }
}

QVariant QmlWrapper::readProperty(const QString& propertyName) {
    if (QThread::currentThread() != thread()) {
        QVariant result;
        BLOCKING_INVOKE_METHOD(this, "readProperty", Q_RETURN_ARG(QVariant, result), Q_ARG(QString, propertyName));
        return result;
    }

    return _qmlObject->property(propertyName.toStdString().c_str());
}

QVariant QmlWrapper::readProperties(const QVariant& propertyList) {
    if (QThread::currentThread() != thread()) {
        QVariant result;
        BLOCKING_INVOKE_METHOD(this, "readProperties", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, propertyList));
        return result;
    }

    QVariantMap result;
    for (const QVariant& property : propertyList.toList()) {
        QString propertyString = property.toString();
        result.insert(propertyString, _qmlObject->property(propertyString.toStdString().c_str()));
    }
    return result;
}
