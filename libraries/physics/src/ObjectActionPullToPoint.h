//
//  ObjectActionPullToPoint.h
//  libraries/physics/src
//
//  Created by Seth Alves 2015-6-3
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ObjectActionPullToPoint_h
#define hifi_ObjectActionPullToPoint_h

#include <QUuid>

#include <EntityItem.h>
#include "ObjectAction.h"

class ObjectActionPullToPoint : public ObjectAction {
public:
    ObjectActionPullToPoint(QUuid id, EntityItemPointer ownerEntity);
    virtual ~ObjectActionPullToPoint();

    virtual bool updateArguments(QVariantMap arguments);
    virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep);

private:
    glm::vec3 _target;
    float _speed;
};

#endif // hifi_ObjectActionPullToPoint_h
