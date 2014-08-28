//
//  CollisionInfo.h
//  libraries/shared/src
//
//  Created by Andrew Meadows on 02/14/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CollisionInfo_h
#define hifi_CollisionInfo_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QtGlobal>
#include <QVector>

class Shape;

const quint32 COLLISION_GROUP_ENVIRONMENT = 1U << 0;
const quint32 COLLISION_GROUP_AVATARS     = 1U << 1;
const quint32 COLLISION_GROUP_VOXELS      = 1U << 2;
const quint32 COLLISION_GROUP_PARTICLES   = 1U << 3;
const quint32 VALID_COLLISION_GROUPS = 0x0f;

// CollisionInfo contains details about the collision between two things: BodyA and BodyB.
// The assumption is that the context that analyzes the collision knows about BodyA but
// does not necessarily know about BodyB.  Hence the data storred in the CollisionInfo
// is expected to be relative to BodyA (for example the penetration points from A into B).

class CollisionInfo {
public:
    CollisionInfo();
    ~CollisionInfo() {}

    // TODO: Andrew to get rid of these data members
    void* _data;
    int _intData;       
    float _floatData;
    glm::vec3 _vecData;

    /// accumulates position changes for the shapes in this collision to resolve penetration
    void apply();

    Shape* getShapeA() const { return const_cast<Shape*>(_shapeA); }
    Shape* getShapeB() const { return const_cast<Shape*>(_shapeB); }

    /// \return unique key for shape pair
    quint64 getShapePairKey() const;

    const Shape* _shapeA;  // pointer to shapeA in this collision
    const Shape* _shapeB;  // pointer to shapeB in this collision

    float _damping;           // range [0,1] of friction coeficient
    float _elasticity;        // range [0,1] of energy conservation
    glm::vec3 _contactPoint;  // world-frame point on BodyA that is deepest into BodyB
    glm::vec3 _penetration;   // depth that BodyA penetrates into BodyB
    glm::vec3 _addedVelocity; // velocity of BodyB
};

// CollisionList is intended to be a recycled container.  Fill the CollisionInfo's,
// use them, and then clear them for the next frame or context.

class CollisionList {
public:
    CollisionList(int maxSize);

    /// \return pointer to next collision. NULL if list is full.
    CollisionInfo* getNewCollision();

    /// \forget about collision at the end
    void deleteLastCollision();

    /// \return pointer to collision by index.  NULL if index out of bounds.
    CollisionInfo* getCollision(int index);

    /// \return pointer to last collision on the list.  NULL if list is empty
    CollisionInfo* getLastCollision();

    /// \return true if list is full
    bool isFull() const { return _size == _maxSize; }

    /// \return number of valid collisions
    int size() const { return _size; }

    /// Clear valid collisions.
    void clear();

    CollisionInfo* operator[](int index);

private:
    int _maxSize;   // the container cannot get larger than this
    int _size;      // the current number of valid collisions in the list
    QVector<CollisionInfo> _collisions;
};

#endif // hifi_CollisionInfo_h
