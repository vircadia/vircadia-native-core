//
//  Created by Gabriel Calero & Cristian Duarte on Aug 25, 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QmlFragmentClass.h"

#include <QtCore/QThread>
#include <QtScript/QScriptContext>
#include <QtScript/QScriptEngine>

#include <shared/QtHelpers.h>

std::mutex QmlFragmentClass::_mutex;
std::map<QString, QScriptValue> QmlFragmentClass::_fragments;

QmlFragmentClass::QmlFragmentClass(bool restricted, QString id) : QmlWindowClass(restricted), qml(id) { }

// Method called by Qt scripts to create a new bottom menu bar in Android
QScriptValue QmlFragmentClass::internal_constructor(QScriptContext* context, QScriptEngine* engine, bool restricted) {
#ifndef DISABLE_QML
    std::lock_guard<std::mutex> guard(_mutex);
    auto qml = context->argument(0).toVariant().toMap().value("qml");
    if (qml.isValid()) {
        // look up tabletId in the map.
        auto iter = _fragments.find(qml.toString());
        if (iter != _fragments.end()) {
            //qDebug() << "[QML-ANDROID] QmlFragmentClass menu already exists";
            return iter->second;
        }
    } else {
        qWarning() << "QmlFragmentClass could not build instance " << qml;
        return QScriptValue();
    }

    auto properties = parseArguments(context);
    QmlFragmentClass* retVal = new QmlFragmentClass(restricted, qml.toString());
    Q_ASSERT(retVal);
    if (QThread::currentThread() != qApp->thread()) {
        retVal->moveToThread(qApp->thread());
        BLOCKING_INVOKE_METHOD(retVal, "initQml", Q_ARG(QVariantMap, properties));
    } else {
        retVal->initQml(properties);
    }
    connect(engine, &QScriptEngine::destroyed, retVal, &QmlWindowClass::deleteLater);
    QScriptValue scriptObject = engine->newQObject(retVal);
    _fragments[qml.toString()] = scriptObject;
    return scriptObject;
#else
    return QScriptValue();
#endif
}

void QmlFragmentClass::close() {
    QmlWindowClass::close();
    _fragments.erase(qml);
}

QObject* QmlFragmentClass::addButton(const QVariant& properties) {
#ifndef DISABLE_QML
    QVariant resultVar;
    Qt::ConnectionType connectionType = Qt::AutoConnection;
    
    if (QThread::currentThread() != _qmlWindow->thread()) {
        connectionType = Qt::BlockingQueuedConnection;
    }
    bool hasResult = QMetaObject::invokeMethod(_qmlWindow, "addButton", connectionType,
                                               Q_RETURN_ARG(QVariant, resultVar), Q_ARG(QVariant, properties));
    if (!hasResult) {
        qWarning() << "QmlFragmentClass addButton has no result";
        return NULL;
    }

    QObject* qmlButton = qvariant_cast<QObject *>(resultVar);
    if (!qmlButton) {
        qWarning() << "QmlFragmentClass addButton result not a QObject";
        return NULL;
    }
    return qmlButton;
#else
    return nullptr;
#endif
}

void QmlFragmentClass::removeButton(QObject* button) {
}
