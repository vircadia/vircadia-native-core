//
//  ScriptUUID.h
//  hifi
//
//  Created by Andrew Meadows on 2014.04.07
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include <QDebug>

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
    qDebug() << qPrintable(lable) << id.toString();
}
