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

#include <AACube.h>

#ifdef USE_BULLET_PHYSICS
#include "ObjectMotionState.h"
#else // USE_BULLET_PHYSICS
// ObjectMotionState stubbery
class ObjectMotionState {
public:
    bool _foo;
};
#endif // USE_BULLET_PHYSICS

class EntityItem;

// From the MotionState's perspective:
//      Inside = physics simulation
//      Outside = external agents (scripts, user interaction, other simulations)

class EntityMotionState : public ObjectMotionState {
public:
    // The WorldOffset is used to keep the positions of objects in the simulation near the origin, to
    // reduce numerical error when computing vector differences.  In other words: The EntityMotionState 
    // class translates between the simulation-frame and the world-frame as known by the render pipeline, 
    // various object trees, etc.  The EntityMotionState class uses a static "worldOffset" to help in
    // the translations.
    static void setWorldOffset(const glm::vec3& offset);
    static const glm::vec3& getWorldOffset();

    EntityMotionState(EntityItem* item);
    virtual ~EntityMotionState();

    /// \return MOTION_TYPE_DYNAMIC or MOTION_TYPE_STATIC based on params set in EntityItem
    MotionType computeMotionType() const;

#ifdef USE_BULLET_PHYSICS
    // this relays incoming position/rotation to the RigidBody
    void getWorldTransform (btTransform &worldTrans) const;

    // this relays outgoing position/rotation to the EntityItem
    void setWorldTransform (const btTransform &worldTrans);
#endif // USE_BULLET_PHYSICS

    // these relay incoming values to the RigidBody
    void applyVelocities() const;
    void applyGravity() const;

    void computeShapeInfo(ShapeInfo& info);

    void sendUpdate(OctreeEditPacketSender* packetSender);

protected:
    EntityItem* _entity;
    uint32_t _outgoingPhysicsDirtyFlags;
};

#endif // hifi_EntityMotionState_h
