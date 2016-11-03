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

AssignmentAction::AssignmentAction(EntityActionType type, const QUuid& id, EntityItemPointer ownerEntity) :
    EntityActionInterface(type, id),
    _data(QByteArray()),
    _active(false),
    _ownerEntity(ownerEntity) {
}

AssignmentAction::~AssignmentAction() {
}

void AssignmentAction::removeFromSimulation(EntitySimulationPointer simulation) const {
    withReadLock([&]{
        simulation->removeAction(_id);
        simulation->applyActionChanges();
    });
}

QByteArray AssignmentAction::serialize() const {
    QByteArray result;
    withReadLock([&]{
        result = _data;
    });
    return result;
}

void AssignmentAction::deserialize(QByteArray serializedArguments) {
    withWriteLock([&]{
        _data = serializedArguments;
    });
}

bool AssignmentAction::updateArguments(QVariantMap arguments) {
    qDebug() << "UNEXPECTED -- AssignmentAction::updateArguments called in assignment-client.";
    return false;
}

QVariantMap AssignmentAction::getArguments() {
    qDebug() << "UNEXPECTED -- AssignmentAction::getArguments called in assignment-client.";
    return QVariantMap();
}

glm::vec3 AssignmentAction::getPosition() {
    qDebug() << "UNEXPECTED -- AssignmentAction::getPosition called in assignment-client.";
    return glm::vec3(0.0f);
}

void AssignmentAction::setPosition(glm::vec3 position) {
    qDebug() << "UNEXPECTED -- AssignmentAction::setPosition called in assignment-client.";
}

glm::quat AssignmentAction::getRotation() {
    qDebug() << "UNEXPECTED -- AssignmentAction::getRotation called in assignment-client.";
    return glm::quat();
}

void AssignmentAction::setRotation(glm::quat rotation) {
    qDebug() << "UNEXPECTED -- AssignmentAction::setRotation called in assignment-client.";
}

glm::vec3 AssignmentAction::getLinearVelocity() {
    qDebug() << "UNEXPECTED -- AssignmentAction::getLinearVelocity called in assignment-client.";
    return glm::vec3(0.0f);
}

void AssignmentAction::setLinearVelocity(glm::vec3 linearVelocity) {
    qDebug() << "UNEXPECTED -- AssignmentAction::setLinearVelocity called in assignment-client.";
}

glm::vec3 AssignmentAction::getAngularVelocity() {
    qDebug() << "UNEXPECTED -- AssignmentAction::getAngularVelocity called in assignment-client.";
    return glm::vec3(0.0f);
}

void AssignmentAction::setAngularVelocity(glm::vec3 angularVelocity) {
    qDebug() << "UNEXPECTED -- AssignmentAction::setAngularVelocity called in assignment-client.";
}
