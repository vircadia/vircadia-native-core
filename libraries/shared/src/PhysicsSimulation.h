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
#include <QMap>
#include <QVector>

#include "CollisionInfo.h"
#include "ContactPoint.h"

class PhysicsEntity;
class Ragdoll;

class PhysicsSimulation {
public:

    PhysicsSimulation();
    ~PhysicsSimulation();

    /// \return true if entity was added to or is already in the list
    bool addEntity(PhysicsEntity* entity);

    void removeEntity(PhysicsEntity* entity);
    void removeShapes(const PhysicsEntity* entity);

    /// \return true if doll was added to or is already in the list
    bool addRagdoll(Ragdoll* doll);

    void removeRagdoll(Ragdoll* doll);

    /// \param minError constraint motion below this value is considered "close enough"
    /// \param maxIterations max number of iterations before giving up
    /// \param maxUsec max number of usec to spend enforcing constraints
    /// \return distance of largest movement
    void stepForward(float deltaTime, float minError, int maxIterations, quint64 maxUsec);

protected:
    void moveRagdolls(float deltaTime);
    void computeCollisions();
    void resolveCollisions();

    void buildContactConstraints();
    void enforceContactConstraints();
    void updateContacts();
    void pruneContacts();

private:
    quint32 _frame;

    QVector<Ragdoll*> _dolls;
    QVector<PhysicsEntity*> _entities;
    CollisionList _collisions;
    QMap<quint64, ContactPoint> _contacts;
};

#endif // hifi_PhysicsSimulation
