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
        // Special case to handle raising and lowering the virtual keyboard.
        const QString RAISE_KEYBOARD = "_RAISE_KEYBOARD";
        const QString RAISE_KEYBOARD_NUMERIC = "_RAISE_KEYBOARD_NUMERIC";
        const QString LOWER_KEYBOARD = "_LOWER_KEYBOARD";
        QString messageString = webMessage.type() == QVariant::String ? webMessage.toString() : "";
        if (messageString.left(RAISE_KEYBOARD.length()) == RAISE_KEYBOARD) {
            setKeyboardRaised(asQuickItem(), true, messageString == RAISE_KEYBOARD_NUMERIC);
        } else if (messageString == LOWER_KEYBOARD) {
            setKeyboardRaised(asQuickItem(), false);
        } else {
            emit webEventReceived(webMessage);
        }
    }
}

void QmlWebWindowClass::setKeyboardRaised(QObject* object, bool raised, bool numeric) {
    if (!object) {
        return;
    }

    QQuickItem* item = dynamic_cast<QQuickItem*>(object);
    while (item) {
        if (item->property("keyboardRaised").isValid()) {
            if (item->property("punctuationMode").isValid()) {
                item->setProperty("punctuationMode", QVariant(numeric));
            }
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
