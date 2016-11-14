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
static const char* const SCRIPT_PROPERTY = "scriptUrl";

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

QString QmlWebWindowClass::getURL() const {
    QVariant result = DependencyManager::get<OffscreenUi>()->returnFromUiThread([&]()->QVariant {
        if (_qmlWindow.isNull()) {
            return QVariant();
        }
        return _qmlWindow->property(URL_PROPERTY);
    });
    return result.toString();
}

void QmlWebWindowClass::setURL(const QString& urlString) {
    DependencyManager::get<OffscreenUi>()->executeOnUiThread([=] {
        if (!_qmlWindow.isNull()) {
            _qmlWindow->setProperty(URL_PROPERTY, urlString);
        }
    });
}

void QmlWebWindowClass::setScriptUrl(const QString& script) {
    DependencyManager::get<OffscreenUi>()->executeOnUiThread([=] {
        if (!_qmlWindow.isNull()) {
            _qmlWindow->setProperty(SCRIPT_PROPERTY, script);
        }
    });
}

void QmlWebWindowClass::clearScriptUrl(const QString& script) {
    setScriptUrl("");
}
