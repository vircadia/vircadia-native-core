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

#include <btBulletDynamicsCommon.h>

#include <QUuid>

#include <EntityItem.h>

class ObjectAction : public btActionInterface, public EntityActionInterface {
public:
    ObjectAction(QUuid id, EntityItemPointer ownerEntity);
    virtual ~ObjectAction();

    const QUuid& getID() const { return _id; }
    virtual const EntityItemPointer& getOwnerEntity() const { return _ownerEntity; }
    virtual void setOwnerEntity(const EntityItemPointer ownerEntity) { _ownerEntity = ownerEntity; }

    virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep);

    virtual void debugDraw(btIDebugDraw* debugDrawer);

private:
    QUuid _id;
    EntityItemPointer _ownerEntity;
};

#endif // hifi_ObjectAction_h
