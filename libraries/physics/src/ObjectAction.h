//
//  ObjectAction.h
//  libraries/physcis/src
//
//  Created by Seth Alves 2015.6.2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ObjectAction_h
#define hifi_ObjectAction_h

#include <btBulletDynamicsCommon.h>

#include <QUuid>

#include <EntityItem.h>

// http://bulletphysics.org/Bullet/BulletFull/classbtActionInterface.html

class ObjectAction : public btActionInterface {
public:
    ObjectAction(EntityItemPointer ownerEntity);
    virtual ~ObjectAction();

    const QUuid& getID() const { return _id; }

    // virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep) = 0

private:
    QUuid _id;
    EntityItemPointer _ownerEntity;
};

#endif // hifi_ObjectAction_h
