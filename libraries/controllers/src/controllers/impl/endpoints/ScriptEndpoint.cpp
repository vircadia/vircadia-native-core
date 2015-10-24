//
//  Created by Bradley Austin Davis 2015/10/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptEndpoint.h"

#include <QtCore/QThread>

using namespace controller;

float ScriptEndpoint::value() {
    updateValue();
    return _lastValue;
}

void ScriptEndpoint::updateValue() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "updateValue", Qt::QueuedConnection);
        return;
    }

    _lastValue = (float)_callable.call().toNumber();
}

void ScriptEndpoint::apply(float newValue, float oldValue, const Pointer& source) {
    internalApply(newValue, oldValue, source->getInput().getID());
}

void ScriptEndpoint::internalApply(float newValue, float oldValue, int sourceID) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "internalApply", Qt::QueuedConnection,
            Q_ARG(float, newValue),
            Q_ARG(float, oldValue),
            Q_ARG(int, sourceID));
        return;
    }
    _callable.call(QScriptValue(),
        QScriptValueList({ QScriptValue(newValue), QScriptValue(oldValue), QScriptValue(sourceID) }));
}
