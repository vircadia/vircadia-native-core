//
//  PhysicsSimulation.h
//  interface/src/avatar
//
//  Created by Andrew Meadows 2014.06.06
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PhysicsSimulation
#define hifi_PhysicsSimulation

#include <QtGlobal>
#include <QVector>

#include "CollisionInfo.h"

class PhysicsEntity;
class Ragdoll;

class PhysicsSimulation {
public:

    PhysicsSimulation();
    ~PhysicsSimulation();

    /// \return true if entity was added to or is already in the list
    bool addEntity(PhysicsEntity* entity);

    void removeEntity(PhysicsEntity* entity);

    /// \return true if doll was added to or is already in the list
    bool addRagdoll(Ragdoll* doll);

    void removeRagdoll(Ragdoll* doll);

    /// \param minError constraint motion below this value is considered "close enough"
    /// \param maxIterations max number of iterations before giving up
    /// \param maxUsec max number of usec to spend enforcing constraints
    /// \return distance of largest movement
    void stepForward(float deltaTime, float minError, int maxIterations, quint64 maxUsec);

    void moveRagdolls(float deltaTime);
    void computeCollisions();
    void processCollisions();

private:
    CollisionList _collisionList;
    QVector<PhysicsEntity*> _entities;
    QVector<Ragdoll*> _dolls;

    // some stats
    quint32 _frame;
    int _numIterations;
    int _numCollisions;
    float _constraintError;
    quint64 _stepTime;
};

#endif // hifi_PhysicsSimulation
