//
//  Created by Bradley Austin Davis on 2015-12-15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QmlWebWindowClass.h"

#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtCore/QThread>

#include <QtQml/QQmlContext>

#include <QtScript/QScriptContext>
#include <QtScript/QScriptEngine>

#include <QtQuick/QQuickItem>

#include <AbstractUriHandler.h>
#include <AccountManager.h>
#include <AddressManager.h>
#include <DependencyManager.h>

#include "OffscreenUi.h"

static const char* const URL_PROPERTY = "source";

// Method called by Qt scripts to create a new web window in the overlay
QScriptValue QmlWebWindowClass::constructor(QScriptContext* context, QScriptEngine* engine) {
    return QmlWindowClass::internalConstructor("QmlWebWindow.qml", context, engine,
        [&](QQmlContext* context, QObject* object) { return new QmlWebWindowClass(object); });
}

QmlWebWindowClass::QmlWebWindowClass(QObject* qmlWindow) : QmlWindowClass(qmlWindow) {
    QObject::connect(_qmlWindow, SIGNAL(navigating(QString)), this, SLOT(handleNavigation(QString)));
}

void QmlWebWindowClass::handleNavigation(const QString& url) {
    bool handled = false;
    static auto handler = dynamic_cast<AbstractUriHandler*>(qApp);
    if (handler) {
        if (handler->canAcceptURL(url)) {
            handled = handler->acceptURL(url);
        }
    }

    if (handled) {
        QMetaObject::invokeMethod(_qmlWindow, "stop", Qt::AutoConnection);
    }
}

QString QmlWebWindowClass::getURL() const { 
    if (QThread::currentThread() != thread()) {
        QString result;
        QMetaObject::invokeMethod(const_cast<QmlWebWindowClass*>(this), "getURL", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QString, result));
        return result;
    }

    return _qmlWindow->property(URL_PROPERTY).toString();
}

void QmlWebWindowClass::setURL(const QString& urlString) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setURL", Qt::QueuedConnection, Q_ARG(QString, urlString));
    }
    _qmlWindow->setProperty(URL_PROPERTY, urlString);
}