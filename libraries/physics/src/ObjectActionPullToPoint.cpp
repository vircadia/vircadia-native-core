//
//  ObjectActionPullToPoint.cpp
//  libraries/physics/src
//
//  Created by Seth Alves 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ObjectActionPullToPoint.h"

ObjectActionPullToPoint::ObjectActionPullToPoint(QUuid id, EntityItemPointer ownerEntity, glm::vec3 target) :
    ObjectAction(id, ownerEntity),
    _target(target) {
}

ObjectActionPullToPoint::~ObjectActionPullToPoint() {
}

void ObjectActionPullToPoint::updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep) {
    qDebug() << "ObjectActionPullToPoint::updateAction called";
}
