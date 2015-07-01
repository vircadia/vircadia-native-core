//
//  ObjectAction.h
//  libraries/physcis/src
//
//  Created by Seth Alves 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  http://bulletphysics.org/Bullet/BulletFull/classbtActionInterface.html

#ifndef hifi_ObjectAction_h
#define hifi_ObjectAction_h

#include <QUuid>

#include <btBulletDynamicsCommon.h>

#include <EntityItem.h>

#include "ObjectMotionState.h"
#include "BulletUtil.h"
#include "EntityActionInterface.h"


class ObjectAction : public btActionInterface, public EntityActionInterface {
public:
    ObjectAction(EntityActionType type, QUuid id, EntityItemPointer ownerEntity);
    virtual ~ObjectAction();

    const QUuid& getID() const { return _id; }
    virtual EntityActionType getType() { assert(false); return ACTION_TYPE_NONE; }
    virtual void removeFromSimulation(EntitySimulation* simulation) const;
    virtual EntityItemWeakPointer getOwnerEntity() const { return _ownerEntity; }
    virtual void setOwnerEntity(const EntityItemPointer ownerEntity) { _ownerEntity = ownerEntity; }

    virtual bool updateArguments(QVariantMap arguments) { return false; }
    virtual QVariantMap getArguments() { return QVariantMap(); }

    // this is called from updateAction and should be overridden by subclasses
    virtual void updateActionWorker(float deltaTimeStep) {}

    // these are from btActionInterface
    virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep);
    virtual void debugDraw(btIDebugDraw* debugDrawer);

    virtual QByteArray serialize();
    virtual void deserialize(QByteArray serializedArguments);

private:
    QUuid _id;
    QReadWriteLock _lock;

protected:

    virtual btRigidBody* getRigidBody();
    virtual glm::vec3 getPosition();
    virtual void setPosition(glm::vec3 position);
    virtual glm::quat getRotation();
    virtual void setRotation(glm::quat rotation);
    virtual glm::vec3 getLinearVelocity();
    virtual void setLinearVelocity(glm::vec3 linearVelocity);
    virtual glm::vec3 getAngularVelocity();
    virtual void setAngularVelocity(glm::vec3 angularVelocity);

    void lockForRead() { _lock.lockForRead(); }
    bool tryLockForRead() { return _lock.tryLockForRead(); }
    void lockForWrite() { _lock.lockForWrite(); }
    bool tryLockForWrite() { return _lock.tryLockForWrite(); }
    void unlock() { _lock.unlock(); }

    bool _active;
    EntityItemWeakPointer _ownerEntity;
};

#endif // hifi_ObjectAction_h
