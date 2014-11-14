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

#include <AACube.h>
#include <CustomMotionState.h>

class EntityItem;

class EntityMotionState : public CustomMotionState {
public:
    static void setWorldOffset(const glm::vec3& offset);
    static const glm::vec3& getWorldOffset();

    EntityMotionState(EntityItem* item);
    virtual ~EntityMotionState();

    MotionType getMotionType() const;

    void getWorldTransform (btTransform &worldTrans) const;
    void setWorldTransform (const btTransform &worldTrans);
    void applyVelocities() const;

    void computeShapeInfo(ShapeInfo& info);

    void getBoundingCubes(AACube& oldCube, AACube& newCube);

protected:
    EntityItem* _entity;
    AACube _oldBoundingCube;
};

#endif // USE_BULLET_PHYSICS
#endif // hifi_EntityMotionState_h
