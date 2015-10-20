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
    bool ownerEntityExpired = false;
    quint64 expiresWhen = 0;

    withReadLock([&]{
        ownerEntityExpired = _ownerEntity.expired();
        expiresWhen = _expires;
    });

    if (ownerEntityExpired) {
        qDebug() << "warning -- action with no entity removing self from btCollisionWorld.";
        btDynamicsWorld* dynamicsWorld = static_cast<btDynamicsWorld*>(collisionWorld);
        dynamicsWorld->removeAction(this);
        return;
    }

    if (expiresWhen > 0) {
        quint64 now = usecTimestampNow();
        if (now > expiresWhen) {
            EntityItemPointer ownerEntity = nullptr;
            QUuid myID;
            withWriteLock([&]{
                ownerEntity = _ownerEntity.lock();
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

    updateActionWorker(deltaTimeStep);
}

int ObjectAction::getEntityServerClockSkew() const {
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

bool ObjectAction::updateArguments(QVariantMap arguments) {
    bool somethingChanged = false;

    withWriteLock([&]{
        quint64 previousExpires = _expires;
        QString previousTag = _tag;

        bool lifetimeSet = true;
        float lifetime = EntityActionInterface::extractFloatArgument("action", arguments, "lifetime", lifetimeSet, false);
        if (lifetimeSet) {
            quint64 now = usecTimestampNow();
            _expires = now + (quint64)(lifetime * USECS_PER_SECOND);
        } else {
            _expires = 0;
        }

        bool tagSet = true;
        QString tag = EntityActionInterface::extractStringArgument("action", arguments, "tag", tagSet, false);
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

QVariantMap ObjectAction::getArguments() {
    QVariantMap arguments;
    withReadLock([&]{
        if (_expires == 0) {
            arguments["lifetime"] = 0.0f;
        } else {
            quint64 now = usecTimestampNow();
            arguments["lifetime"] = (float)(_expires - now) / (float)USECS_PER_SECOND;
        }
        arguments["tag"] = _tag;
    });
    return arguments;
}


void ObjectAction::debugDraw(btIDebugDraw* debugDrawer) {
}

void ObjectAction::removeFromSimulation(EntitySimulation* simulation) const {
    QUuid myID;
    withReadLock([&]{
        myID = _id;
    });
    simulation->removeAction(myID);
}

btRigidBody* ObjectAction::getRigidBody() {
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
    if (_expires == 0) {
        return false;
    }

    quint64 now = usecTimestampNow();
    if (now >= _expires) {
        return true;
    }
    return false;
}
