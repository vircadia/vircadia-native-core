//
//  AssignmentAction.cpp
//  assignment-client/src/
//
//  Created by Seth Alves 2015-6-19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntitySimulation.h"

#include "AssignmentAction.h"

AssignmentAction::AssignmentAction(EntityActionType type, QUuid id, EntityItemPointer ownerEntity) :
    _id(id),
    _type(type),
    _data(QByteArray()),
    _active(false),
    _ownerEntity(ownerEntity) {
}

AssignmentAction::~AssignmentAction() {
}

void AssignmentAction::removeFromSimulation(EntitySimulation* simulation) const {
    simulation->removeAction(_id);
}

QByteArray AssignmentAction::serialize() {
    return _data;
}

void AssignmentAction::deserialize(QByteArray serializedArguments) {
    _data = serializedArguments;
}
