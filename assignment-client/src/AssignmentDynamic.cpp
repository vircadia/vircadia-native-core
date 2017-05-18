//
//  AssignmentDynamic.cpp
//  assignment-client/src/
//
//  Created by Seth Alves 2015-6-19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntitySimulation.h"

#include "AssignmentDynamic.h"

AssignmentDynamic::AssignmentDynamic(EntityDynamicType type, const QUuid& id, EntityItemPointer ownerEntity) :
    EntityDynamicInterface(type, id),
    _data(QByteArray()),
    _active(false),
    _ownerEntity(ownerEntity) {
}

AssignmentDynamic::~AssignmentDynamic() {
}

void AssignmentDynamic::removeFromSimulation(EntitySimulationPointer simulation) const {
    withReadLock([&]{
        simulation->removeDynamic(_id);
        simulation->applyDynamicChanges();
    });
}

QByteArray AssignmentDynamic::serialize() const {
    QByteArray result;
    withReadLock([&]{
        result = _data;
    });
    return result;
}

void AssignmentDynamic::deserialize(QByteArray serializedArguments) {
    withWriteLock([&]{
        _data = serializedArguments;
    });
}

bool AssignmentDynamic::updateArguments(QVariantMap arguments) {
    qDebug() << "UNEXPECTED -- AssignmentDynamic::updateArguments called in assignment-client.";
    return false;
}

QVariantMap AssignmentDynamic::getArguments() {
    qDebug() << "UNEXPECTED -- AssignmentDynamic::getArguments called in assignment-client.";
    return QVariantMap();
}
