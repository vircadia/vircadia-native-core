//
//  ObjectAction.h
//  libraries/physics/src
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
#include "ObjectDynamic.h"

class ObjectAction : public btActionInterface, public ObjectDynamic {
public:
    ObjectAction(EntityDynamicType type, const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~ObjectAction() {}

    virtual bool isAction() const override { return true; }

    // this is called from updateAction and should be overridden by subclasses
    virtual void updateActionWorker(float deltaTimeStep) = 0;

    // these are from btActionInterface
    virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep) override;
    virtual void debugDraw(btIDebugDraw* debugDrawer) override;
};

#endif // hifi_ObjectAction_h
