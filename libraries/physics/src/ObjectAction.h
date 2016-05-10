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

#include <shared/ReadWriteLockable.h>

#include "ObjectMotionState.h"
#include "BulletUtil.h"
#include "EntityActionInterface.h"


class ObjectAction : public btActionInterface, public EntityActionInterface, public ReadWriteLockable {
public:
    ObjectAction(EntityActionType type, const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~ObjectAction();

    virtual void removeFromSimulation(EntitySimulationPointer simulation) const override;
    virtual EntityItemWeakPointer getOwnerEntity() const override { return _ownerEntity; }
    virtual void setOwnerEntity(const EntityItemPointer ownerEntity) override { _ownerEntity = ownerEntity; }

    virtual bool updateArguments(QVariantMap arguments) override;
    virtual QVariantMap getArguments() override;

    // this is called from updateAction and should be overridden by subclasses
    virtual void updateActionWorker(float deltaTimeStep) = 0;

    // these are from btActionInterface
    virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep) override;
    virtual void debugDraw(btIDebugDraw* debugDrawer) override;

    virtual QByteArray serialize() const override = 0;
    virtual void deserialize(QByteArray serializedArguments) override = 0;

    virtual bool lifetimeIsOver() override;
    virtual quint64 getExpires() override { return _expires; }

protected:
    quint64 localTimeToServerTime(quint64 timeValue) const;
    quint64 serverTimeToLocalTime(quint64 timeValue) const;

    virtual btRigidBody* getRigidBody();
    virtual glm::vec3 getPosition() override;
    virtual void setPosition(glm::vec3 position) override;
    virtual glm::quat getRotation() override;
    virtual void setRotation(glm::quat rotation) override;
    virtual glm::vec3 getLinearVelocity() override;
    virtual void setLinearVelocity(glm::vec3 linearVelocity) override;
    virtual glm::vec3 getAngularVelocity() override;
    virtual void setAngularVelocity(glm::vec3 angularVelocity) override;
    virtual void activateBody(bool forceActivation = false);
    virtual void forceBodyNonStatic();

    EntityItemWeakPointer _ownerEntity;
    QString _tag;
    quint64 _expires { 0 }; // in seconds since epoch

private:
    qint64 getEntityServerClockSkew() const;
};

#endif // hifi_ObjectAction_h
