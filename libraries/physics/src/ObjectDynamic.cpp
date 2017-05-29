//
//  ObjectDynamic.cpp
//  libraries/physcis/src
//
//  Created by Seth Alves 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntitySimulation.h"

#include "ObjectDynamic.h"

#include "PhysicsLogging.h"


ObjectDynamic::ObjectDynamic(EntityDynamicType type, const QUuid& id, EntityItemPointer ownerEntity) :
    EntityDynamicInterface(type, id),
    _ownerEntity(ownerEntity) {
}

ObjectDynamic::~ObjectDynamic() {
}

qint64 ObjectDynamic::getEntityServerClockSkew() const {
    auto nodeList = DependencyManager::get<NodeList>();

    auto ownerEntity = _ownerEntity.lock();
    if (!ownerEntity) {
        return 0;
    }

    const QUuid& entityServerNodeID = ownerEntity->getSourceUUID();
    auto entityServerNode = nodeList->nodeWithUUID(entityServerNodeID);
    if (entityServerNode) {
        return entityServerNode->getClockSkewUsec();
    }
    return 0;
}

bool ObjectDynamic::updateArguments(QVariantMap arguments) {
    bool somethingChanged = false;

    withWriteLock([&]{
        quint64 previousExpires = _expires;
        QString previousTag = _tag;

        bool ttlSet = true;
        float ttl = EntityDynamicInterface::extractFloatArgument("dynamic", arguments, "ttl", ttlSet, false);
        if (ttlSet) {
            quint64 now = usecTimestampNow();
            _expires = now + (quint64)(ttl * USECS_PER_SECOND);
        } else {
            _expires = 0;
        }

        bool tagSet = true;
        QString tag = EntityDynamicInterface::extractStringArgument("dynamic", arguments, "tag", tagSet, false);
        if (tagSet) {
            _tag = tag;
        } else {
            tag = "";
        }

        if (previousExpires != _expires || previousTag != _tag) {
            somethingChanged = true;
        }
    });

    return somethingChanged;
}

QVariantMap ObjectDynamic::getArguments() {
    QVariantMap arguments;
    withReadLock([&]{
        if (_expires == 0) {
            arguments["ttl"] = 0.0f;
        } else {
            quint64 now = usecTimestampNow();
            arguments["ttl"] = (float)(_expires - now) / (float)USECS_PER_SECOND;
        }
        arguments["tag"] = _tag;

        EntityItemPointer entity = _ownerEntity.lock();
        if (entity) {
            ObjectMotionState* motionState = static_cast<ObjectMotionState*>(entity->getPhysicsInfo());
            if (motionState) {
                arguments["::active"] = motionState->isActive();
                arguments["::motion-type"] = motionTypeToString(motionState->getMotionType());
            } else {
                arguments["::no-motion-state"] = true;
            }
        }
        arguments["isMine"] = isMine();
    });
    return arguments;
}

void ObjectDynamic::removeFromSimulation(EntitySimulationPointer simulation) const {
    QUuid myID;
    withReadLock([&]{
        myID = _id;
    });
    simulation->removeDynamic(myID);
}

EntityItemPointer ObjectDynamic::getEntityByID(EntityItemID entityID) const {
    EntityItemPointer ownerEntity;
    withReadLock([&]{
        ownerEntity = _ownerEntity.lock();
    });
    EntityTreeElementPointer element = ownerEntity ? ownerEntity->getElement() : nullptr;
    EntityTreePointer tree = element ? element->getTree() : nullptr;
    if (!tree) {
        return nullptr;
    }
    return tree->findEntityByID(entityID);
}


btRigidBody* ObjectDynamic::getRigidBody() {
    ObjectMotionState* motionState = nullptr;
    withReadLock([&]{
        auto ownerEntity = _ownerEntity.lock();
        if (!ownerEntity) {
            return;
        }
        void* physicsInfo = ownerEntity->getPhysicsInfo();
        if (!physicsInfo) {
            return;
        }
        motionState = static_cast<ObjectMotionState*>(physicsInfo);
    });
    if (motionState) {
        return motionState->getRigidBody();
    }
    return nullptr;
}

glm::vec3 ObjectDynamic::getPosition() {
    auto rigidBody = getRigidBody();
    if (!rigidBody) {
        return glm::vec3(0.0f);
    }
    return bulletToGLM(rigidBody->getCenterOfMassPosition());
}

glm::quat ObjectDynamic::getRotation() {
    auto rigidBody = getRigidBody();
    if (!rigidBody) {
        return glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
    }
    return bulletToGLM(rigidBody->getOrientation());
}

glm::vec3 ObjectDynamic::getLinearVelocity() {
    auto rigidBody = getRigidBody();
    if (!rigidBody) {
        return glm::vec3(0.0f);
    }
    return bulletToGLM(rigidBody->getLinearVelocity());
}

void ObjectDynamic::setLinearVelocity(glm::vec3 linearVelocity) {
    auto rigidBody = getRigidBody();
    if (!rigidBody) {
        return;
    }
    rigidBody->setLinearVelocity(glmToBullet(glm::vec3(0.0f)));
    rigidBody->activate();
}

glm::vec3 ObjectDynamic::getAngularVelocity() {
    auto rigidBody = getRigidBody();
    if (!rigidBody) {
        return glm::vec3(0.0f);
    }
    return bulletToGLM(rigidBody->getAngularVelocity());
}

void ObjectDynamic::setAngularVelocity(glm::vec3 angularVelocity) {
    auto rigidBody = getRigidBody();
    if (!rigidBody) {
        return;
    }
    rigidBody->setAngularVelocity(glmToBullet(angularVelocity));
    rigidBody->activate();
}

void ObjectDynamic::activateBody(bool forceActivation) {
    auto rigidBody = getRigidBody();
    if (rigidBody) {
        rigidBody->activate(forceActivation);
    } else {
        qCDebug(physics) << "ObjectDynamic::activateBody -- no rigid body" << (void*)rigidBody;
    }
}

void ObjectDynamic::forceBodyNonStatic() {
    auto ownerEntity = _ownerEntity.lock();
    if (!ownerEntity) {
        return;
    }
    void* physicsInfo = ownerEntity->getPhysicsInfo();
    ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
    if (motionState && motionState->getMotionType() == MOTION_TYPE_STATIC) {
        ownerEntity->flagForMotionStateChange();
    }
}

bool ObjectDynamic::lifetimeIsOver() {
    if (_expires == 0) {
        return false;
    }

    quint64 now = usecTimestampNow();
    if (now >= _expires) {
        return true;
    }
    return false;
}

quint64 ObjectDynamic::localTimeToServerTime(quint64 timeValue) const {
    // 0 indicates no expiration
    if (timeValue == 0) {
        return 0;
    }

    qint64 serverClockSkew = getEntityServerClockSkew();
    if (serverClockSkew < 0 && timeValue <= (quint64)(-serverClockSkew)) {
        return 1; // non-zero but long-expired value to avoid negative roll-over
    }

    return timeValue + serverClockSkew;
}

quint64 ObjectDynamic::serverTimeToLocalTime(quint64 timeValue) const {
    // 0 indicates no expiration
    if (timeValue == 0) {
        return 0;
    }

    qint64 serverClockSkew = getEntityServerClockSkew();
    if (serverClockSkew > 0 && timeValue <= (quint64)serverClockSkew) {
        return 1; // non-zero but long-expired value to avoid negative roll-over
    }

    return timeValue - serverClockSkew;
}

btRigidBody* ObjectDynamic::getOtherRigidBody(EntityItemID otherEntityID) {
    EntityItemPointer otherEntity = getEntityByID(otherEntityID);
    if (!otherEntity) {
        return nullptr;
    }

    void* otherPhysicsInfo = otherEntity->getPhysicsInfo();
    if (!otherPhysicsInfo) {
        return nullptr;
    }

    ObjectMotionState* otherMotionState = static_cast<ObjectMotionState*>(otherPhysicsInfo);
    if (!otherMotionState) {
        return nullptr;
    }

    return otherMotionState->getRigidBody();
}

QList<btRigidBody*> ObjectDynamic::getRigidBodies() {
    QList<btRigidBody*> result;
    result += getRigidBody();
    return result;
}
