//
//  Created by Bradley Austin Davis 2015/10/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptEndpoint.h"

#include <QtCore/QThread>

#include <StreamUtils.h>

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

    QScriptValue result = _callable.call();

    // If the callable ever returns a non-number, we assume it's a pose
    // and start reporting ourselves as a pose.
    if (result.isNumber()) {
        _lastValueRead = (float)_callable.call().toNumber();
    } else {
        Pose::fromScriptValue(result, _lastPoseRead);
        _returnPose = true;
    }
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

Pose ScriptEndpoint::peekPose() const {
    const_cast<ScriptEndpoint*>(this)->updatePose();
    return _lastPoseRead;
}

void ScriptEndpoint::updatePose() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "updatePose", Qt::QueuedConnection);
        return;
    }
    QScriptValue result = _callable.call();
    Pose::fromScriptValue(result, _lastPoseRead);
}

void ScriptEndpoint::apply(const Pose& newPose, const Pointer& source) {
    if (newPose == _lastPoseWritten) {
        return;
    }
    internalApply(newPose, source->getInput().getID());
}

void ScriptEndpoint::internalApply(const Pose& newPose, int sourceID) {
    _lastPoseWritten = newPose;
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "internalApply", Qt::QueuedConnection,
            Q_ARG(const Pose&, newPose),
            Q_ARG(int, sourceID));
        return;
    }
    _callable.call(QScriptValue(),
        QScriptValueList({ Pose::toScriptValue(_callable.engine(), newPose), QScriptValue(sourceID) }));
}
