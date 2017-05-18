//
//  ObjectDynamic.h
//  libraries/physcis/src
//
//  Created by Seth Alves 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  http://bulletphysics.org/Bullet/BulletFull/classbtDynamicInterface.html

#ifndef hifi_ObjectDynamic_h
#define hifi_ObjectDynamic_h

#include <QUuid>

#include <btBulletDynamicsCommon.h>

#include <shared/ReadWriteLockable.h>

#include "ObjectMotionState.h"
#include "BulletUtil.h"
#include "EntityDynamicInterface.h"


class ObjectDynamic : public EntityDynamicInterface, public ReadWriteLockable {
public:
    ObjectDynamic(EntityDynamicType type, const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~ObjectDynamic();

    virtual void remapIDs(QHash<EntityItemID, EntityItemID>& map) override;

    virtual void removeFromSimulation(EntitySimulationPointer simulation) const override;
    virtual EntityItemWeakPointer getOwnerEntity() const override { return _ownerEntity; }
    virtual void setOwnerEntity(const EntityItemPointer ownerEntity) override { _ownerEntity = ownerEntity; }

    virtual void invalidate() {};

    virtual bool updateArguments(QVariantMap arguments) override;
    virtual QVariantMap getArguments() override;


    virtual QByteArray serialize() const override = 0;
    virtual void deserialize(QByteArray serializedArguments) override = 0;

    virtual bool lifetimeIsOver() override;
    virtual quint64 getExpires() override { return _expires; }

    virtual QList<btRigidBody*> getRigidBodies();

protected:
    quint64 localTimeToServerTime(quint64 timeValue) const;
    quint64 serverTimeToLocalTime(quint64 timeValue) const;

    btRigidBody* getOtherRigidBody(EntityItemID otherEntityID);
    EntityItemPointer getEntityByID(EntityItemID entityID) const;
    virtual btRigidBody* getRigidBody();
    virtual void activateBody(bool forceActivation = false);
    virtual void forceBodyNonStatic();

    EntityItemWeakPointer _ownerEntity;
    QString _tag;
    quint64 _expires { 0 }; // in seconds since epoch

    EntityItemID _otherID;
    SpatiallyNestableWeakPointer _other;
    SpatiallyNestablePointer getOther();

private:
    qint64 getEntityServerClockSkew() const;
};

typedef std::shared_ptr<ObjectDynamic> ObjectDynamicPointer;

#endif // hifi_ObjectDynamic_h
