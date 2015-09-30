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

#include "EntitySimulation.h"

#include "ObjectAction.h"

ObjectAction::ObjectAction(EntityActionType type, const QUuid& id, EntityItemPointer ownerEntity) :
    btActionInterface(),
    EntityActionInterface(type, id),
    _active(false),
    _ownerEntity(ownerEntity) {
}

ObjectAction::~ObjectAction() {
}

void ObjectAction::updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep) {
    if (_ownerEntity.expired()) {
        qDebug() << "warning -- action with no entity removing self from btCollisionWorld.";
        btDynamicsWorld* dynamicsWorld = static_cast<btDynamicsWorld*>(collisionWorld);
        dynamicsWorld->removeAction(this);
        return;
    }

    if (_expires > 0.0f) {
        float now = (float)usecTimestampNow() / USECS_PER_SECOND;
        if (now > _expires) {
            EntityItemPointer ownerEntity = _ownerEntity.lock();
            _active = false;
            if (ownerEntity) {
                ownerEntity->removeAction(nullptr, getID());
            }
        }
    }

    if (!_active) {
        return;
    }

    updateActionWorker(deltaTimeStep);
}

bool ObjectAction::updateArguments(QVariantMap arguments) {
    bool lifetimeSet = true;
    float lifetime = EntityActionInterface::extractFloatArgument("action", arguments, "lifetime", lifetimeSet, false);
    if (lifetimeSet) {
        float now = (float)usecTimestampNow() / USECS_PER_SECOND;
        _expires = now + lifetime;
    } else {
        _expires = 0.0f;
    }

    bool tagSet = true;
    QString tag = EntityActionInterface::extractStringArgument("action", arguments, "tag", tagSet, false);
    if (tagSet) {
        _tag = tag;
    } else {
        tag = "";
    }

    return true;
}

QVariantMap ObjectAction::getArguments() {
    QVariantMap arguments;
    float now = (float)usecTimestampNow() / USECS_PER_SECOND;
    if (_expires == 0.0f) {
        arguments["lifetime"] = 0.0f;
    } else {
        arguments["lifetime"] = _expires - now;
    }
    arguments["tag"] = _tag;
    return arguments;
}


void ObjectAction::debugDraw(btIDebugDraw* debugDrawer) {
}

void ObjectAction::removeFromSimulation(EntitySimulation* simulation) const {
    simulation->removeAction(_id);
}

btRigidBody* ObjectAction::getRigidBody() {
    auto ownerEntity = _ownerEntity.lock();
    if (!ownerEntity) {
        return nullptr;
    }
    void* physicsInfo = ownerEntity->getPhysicsInfo();
    if (!physicsInfo) {
        return nullptr;
    }
    ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
    return motionState->getRigidBody();
}

glm::vec3 ObjectAction::getPosition() {
    auto rigidBody = getRigidBody();
    if (!rigidBody) {
        return glm::vec3(0.0f);
    }
    return bulletToGLM(rigidBody->getCenterOfMassPosition());
}

void ObjectAction::setPosition(glm::vec3 position) {
    auto rigidBody = getRigidBody();
    if (!rigidBody) {
        return;
    }
    // XXX
    // void setWorldTransform (const btTransform &worldTrans)
    assert(false);
    rigidBody->activate();
}

glm::quat ObjectAction::getRotation() {
    auto rigidBody = getRigidBody();
    if (!rigidBody) {
        return glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
    }
    return bulletToGLM(rigidBody->getOrientation());
}

void ObjectAction::setRotation(glm::quat rotation) {
    auto rigidBody = getRigidBody();
    if (!rigidBody) {
        return;
    }
    // XXX
    // void setWorldTransform (const btTransform &worldTrans)
    assert(false);
    rigidBody->activate();
}

glm::vec3 ObjectAction::getLinearVelocity() {
    auto rigidBody = getRigidBody();
    if (!rigidBody) {
        return glm::vec3(0.0f);
    }
    return bulletToGLM(rigidBody->getLinearVelocity());
}

void ObjectAction::setLinearVelocity(glm::vec3 linearVelocity) {
    auto rigidBody = getRigidBody();
    if (!rigidBody) {
        return;
    }
    rigidBody->setLinearVelocity(glmToBullet(glm::vec3(0.0f)));
    rigidBody->activate();
}

glm::vec3 ObjectAction::getAngularVelocity() {
    auto rigidBody = getRigidBody();
    if (!rigidBody) {
        return glm::vec3(0.0f);
    }
    return bulletToGLM(rigidBody->getAngularVelocity());
}

void ObjectAction::setAngularVelocity(glm::vec3 angularVelocity) {
    auto rigidBody = getRigidBody();
    if (!rigidBody) {
        return;
    }
    rigidBody->setAngularVelocity(glmToBullet(angularVelocity));
    rigidBody->activate();
}

void ObjectAction::activateBody() {
    auto rigidBody = getRigidBody();
    if (rigidBody) {
        rigidBody->activate();
    }
}

bool ObjectAction::lifetimeIsOver() {
    if (_expires == 0.0f) {
        return false;
    }

    float now = (float)usecTimestampNow() / USECS_PER_SECOND;
    if (now >= _expires) {
        return true;
    }
    return false;
}
