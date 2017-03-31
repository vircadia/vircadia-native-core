//
//  CharacterGhostObject.h
//  libraries/physics/src
//
//  Created by Andrew Meadows 2016.08.26
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CharacterGhostObject_h
#define hifi_CharacterGhostObject_h

#include <stdint.h>

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <stdint.h>

#include "CharacterSweepResult.h"
#include "CharacterRayResult.h"

class CharacterGhostShape;

class CharacterGhostObject : public btPairCachingGhostObject {
public:
    CharacterGhostObject() { }
    ~CharacterGhostObject();

    void setCollisionGroupAndMask(int16_t group, int16_t mask);
    void getCollisionGroupAndMask(int16_t& group, int16_t& mask) const;

    void setRadiusAndHalfHeight(btScalar radius, btScalar halfHeight);
    void setUpDirection(const btVector3& up);

    void setCharacterShape(btConvexHullShape* shape);

    void setCollisionWorld(btCollisionWorld* world);

    bool rayTest(const btVector3& start,
            const btVector3& end,
            CharacterRayResult& result) const;

    void refreshOverlappingPairCache();

protected:
    void removeFromWorld();
    void addToWorld();

protected:
    btCollisionWorld* _world { nullptr }; // input, pointer to world
    btScalar _halfHeight { 0.0f };
    btScalar _radius { 0.0f };
    btConvexHullShape* _characterShape { nullptr }; // input, shape of character
    CharacterGhostShape* _ghostShape { nullptr }; // internal, shape whose Aabb is used for overlap cache
    int16_t _collisionFilterGroup { 0 };
    int16_t _collisionFilterMask { 0 };
    bool _inWorld { false }; // internal, was added to world
};

#endif // hifi_CharacterGhostObject_h
