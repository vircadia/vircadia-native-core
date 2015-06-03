//
//  ObjectAction.cpp
//  libraries/physcis/src
//
//  Created by Seth Alves 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ObjectAction.h"

ObjectAction::ObjectAction(QUuid id, EntityItemPointer ownerEntity) :
    btActionInterface(),
    _id(id),
    _ownerEntity(ownerEntity) {
}

ObjectAction::~ObjectAction() {
}

void ObjectAction::updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep) {
    qDebug() << "ObjectAction::updateAction called";
}

void ObjectAction::debugDraw(btIDebugDraw* debugDrawer) {
}
