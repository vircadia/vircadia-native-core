//
//  Created by Timothy Dedischew on 2017/12/21
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MiniPromises.h"
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>

int MiniPromise::metaTypeID = qRegisterMetaType<MiniPromise::Promise>("MiniPromise::Promise");

namespace {
    void promiseFromScriptValue(const QScriptValue& object, MiniPromise::Promise& promise) {
        Q_ASSERT(false);
    }
    QScriptValue promiseToScriptValue(QScriptEngine *engine, const MiniPromise::Promise& promise) {
        return engine->newQObject(promise.get());
    }
}
void MiniPromise::registerMetaTypes(QObject* engine) {
    auto scriptEngine = qobject_cast<QScriptEngine*>(engine);
    qDebug() << "----------------------- MiniPromise::registerMetaTypes ------------" << scriptEngine;
    qScriptRegisterMetaType(scriptEngine, promiseToScriptValue, promiseFromScriptValue);
}
