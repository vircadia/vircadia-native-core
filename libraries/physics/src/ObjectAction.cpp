//
//  ObjectAction.cpp
//  libraries/physics/src
//
//  Created by Seth Alves 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntitySimulation.h"

#include "ObjectAction.h"

#include "PhysicsLogging.h"


ObjectAction::ObjectAction(EntityDynamicType type, const QUuid& id, EntityItemPointer ownerEntity) :
    btActionInterface(),
    ObjectDynamic(type, id, ownerEntity)
{
}

void ObjectAction::updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep) {
    quint64 expiresWhen = 0;
    EntityItemPointer ownerEntity = nullptr;

    withReadLock([&]{
        ownerEntity = _ownerEntity.lock();
        expiresWhen = _expires;
    });

    if (!ownerEntity) {
        qCDebug(physics) << "warning -- action [" << _tag << "] with no entity removing self from btCollisionWorld.";
        btDynamicsWorld* dynamicsWorld = static_cast<btDynamicsWorld*>(collisionWorld);
        if (dynamicsWorld) {
            dynamicsWorld->removeAction(this);
        }
        return;
    }

    if (expiresWhen > 0) {
        quint64 now = usecTimestampNow();
        if (now > expiresWhen) {
            QUuid myID;
            withWriteLock([&]{
                _active = false;
                myID = getID();
            });
            if (ownerEntity) {
                ownerEntity->removeAction(nullptr, myID);
            }
        }
    }

    if (!_active) {
        return;
    }

    if (ownerEntity->getLocked()) {
        return;
    }

    updateActionWorker(deltaTimeStep);
}

void ObjectAction::debugDraw(btIDebugDraw* debugDrawer) {
}
