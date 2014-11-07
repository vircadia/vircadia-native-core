//
//  EntityMotionState.h
//  libraries/entities/src
//
//  Created by Andrew Meadows on 2014.11.06
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityMotionState_h
#define hifi_EntityMotionState_h

#ifdef USE_BULLET_PHYSICS

#include <CustomMotionState.h>

class EntityMotionState : public CustomMotionState {
public:
    static void setWorldOffset(const glm::vec3& offset);
    static const glm::vec3& getWorldOffset();

    EntityMotionState(EntityItem* item);
    virtual ~EntityMotionState();

    virtual void getWorldTransform (btTransform &worldTrans) const;
    virtual void setWorldTransform (const btTransform &worldTrans);

    virtual void computeShapeInfo(ShapeInfo& info);

protected:
    EntityItem* _entity;
};

#endif // USE_BULLET_PHYSICS
#endif // hifi_EntityMotionState_h
