//
//  SimulationEngine.h
//  interface/src/avatar
//
//  Created by Andrew Meadows 2014.06.06
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SimulationEngine_h
#define hifi_SimulationEngine_h

#include <QVector>

#include "CollisionInfo.h"

class PhysicalEntity;
class Ragdoll;

class SimulationEngine {
public:

    SimulationEngine();
    ~SimulationEngine();

    /// \return true if entity was added to or is already in the list
    bool addEntity(PhysicalEntity* entity);

    void removeEntity(PhysicalEntity* entity);

    /// \return true if doll was added to or is already in the list
    bool addRagdoll(Ragdoll* doll);

    void removeRagdoll(Ragdoll* doll);

    /// \param minError constraint motion below this value is considered "close enough"
    /// \param maxIterations max number of iterations before giving up
    /// \param maxUsec max number of usec to spend enforcing constraints
    /// \return distance of largest movement
    void stepForward(float deltaTime, float minError, int maxIterations, quint64 maxUsec);

    int getEnforementIterations() const { return _enforcementIterations; }
    float getEnforcementError() const { return _enforcementError; }
    quint64 getEnforcementTime() const { return _enforcementTime; }

    /// \return number of collisions
    int computeCollisions();

    void processCollisions();

private:
    CollisionList _collisionList;
    QVector<PhysicalEntity*> _entities;
    QVector<Ragdoll*> _dolls;

    // some stats for performance queries
    int _enforcementIterations;
    float _enforcementError;
    quint64 _enforcementTime;
};

#endif // hifi_SimulationEngine_h
