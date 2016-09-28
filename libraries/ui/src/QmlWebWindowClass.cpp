//
//  Created by Bradley Austin Davis on 2015-12-15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QmlWebWindowClass.h"

#include <QtCore/QThread>

#include <QtScript/QScriptContext>
#include <QtScript/QScriptEngine>

#include "OffscreenUi.h"

static const char* const URL_PROPERTY = "source";

// Method called by Qt scripts to create a new web window in the overlay
QScriptValue QmlWebWindowClass::constructor(QScriptContext* context, QScriptEngine* engine) {
    auto properties = parseArguments(context);
    QmlWebWindowClass* retVal { nullptr };
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->executeOnUiThread([&] {
        retVal = new QmlWebWindowClass();
        retVal->initQml(properties);
    }, true);
    Q_ASSERT(retVal);
    connect(engine, &QScriptEngine::destroyed, retVal, &QmlWindowClass::deleteLater);
    return engine->newQObject(retVal);
}

void QmlWebWindowClass::emitScriptEvent(const QVariant& scriptMessage) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "emitScriptEvent", Qt::QueuedConnection, Q_ARG(QVariant, scriptMessage));
    } else {
        emit scriptEventReceived(scriptMessage);
    }
}

void QmlWebWindowClass::emitWebEvent(const QVariant& webMessage) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "emitWebEvent", Qt::QueuedConnection, Q_ARG(QVariant, webMessage));
    } else {
        // Special cases for raising and lowering the virtual keyboard.
        if (webMessage.type() == QVariant::String && webMessage.toString() == "_RAISE_KEYBOARD") {
            setKeyboardRaised(asQuickItem(), true);
        } else if (webMessage.type() == QVariant::String && webMessage.toString() == "_LOWER_KEYBOARD") {
            setKeyboardRaised(asQuickItem(), false);
        } else {
            emit webEventReceived(webMessage);
        }
    }
}

void QmlWebWindowClass::setKeyboardRaised(QObject* object, bool raised) {
    if (!object) {
        return;
    }

    QQuickItem* item = dynamic_cast<QQuickItem*>(object);
    while (item) {
        if (item->property("keyboardRaised").isValid()) {
            item->setProperty("keyboardRaised", QVariant(raised));
            return;
        }
        item = dynamic_cast<QQuickItem*>(item->parentItem());
    }
}

QString QmlWebWindowClass::getURL() const {
    QVariant result = DependencyManager::get<OffscreenUi>()->returnFromUiThread([&]()->QVariant {
        if (_qmlWindow.isNull()) {
            return QVariant();
        }
        return _qmlWindow->property(URL_PROPERTY);
    });
    return result.toString();
}

// HACK find a good place to declare and store this
extern QString fixupHifiUrl(const QString& urlString);

void QmlWebWindowClass::setURL(const QString& urlString) {
    DependencyManager::get<OffscreenUi>()->executeOnUiThread([=] {
        if (!_qmlWindow.isNull()) {
            _qmlWindow->setProperty(URL_PROPERTY, fixupHifiUrl(urlString));
        }
    });
}
