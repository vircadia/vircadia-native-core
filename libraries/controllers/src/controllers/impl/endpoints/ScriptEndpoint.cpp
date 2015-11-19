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

float ScriptEndpoint::peek() const {
    const_cast<ScriptEndpoint*>(this)->updateValue();
    return _lastValueRead;
}

void ScriptEndpoint::updateValue() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "updateValue", Qt::QueuedConnection);
        return;
    }

    _lastValueRead = (float)_callable.call().toNumber();
}

void ScriptEndpoint::apply(float value, const Pointer& source) {
    if (value == _lastValueWritten) {
        return;
    }
    internalApply(value, source->getInput().getID());
}

void ScriptEndpoint::internalApply(float value, int sourceID) {
    _lastValueWritten = value;
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "internalApply", Qt::QueuedConnection,
            Q_ARG(float, value),
            Q_ARG(int, sourceID));
        return;
    }
    _callable.call(QScriptValue(),
        QScriptValueList({ QScriptValue(value), QScriptValue(sourceID) }));
}
