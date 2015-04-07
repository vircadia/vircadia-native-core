//
//  ScriptUUID.cpp
//  libraries/script-engine/src/
//
//  Created by Andrew Meadows on 2014-04-07
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Scriptable interface for a UUID helper class object. Used exclusively in the JavaScript API
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>

#include "ScriptEngineLogging.h"
#include "ScriptUUID.h"

QUuid ScriptUUID::fromString(const QString& s) {
    return QUuid(s);
}

QString ScriptUUID::toString(const QUuid& id) {
    return id.toString();
}

QUuid ScriptUUID::generate() {
    return QUuid::createUuid();    
}

bool ScriptUUID::isEqual(const QUuid& idA, const QUuid& idB) {
    return idA == idB;
}

bool ScriptUUID::isNull(const QUuid& id) {
    return id.isNull();
}

void ScriptUUID::print(const QString& lable, const QUuid& id) {
    qCDebug(scriptengine) << qPrintable(lable) << id.toString();
}
