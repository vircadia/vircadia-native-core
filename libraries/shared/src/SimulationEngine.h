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
#include "RagDoll.h"

class SimulationEngine {
public:

    SimulationEngine();
    ~SimulationEngine();

    /// \return true if doll was added to, or already in the list
    bool addRagDoll(RagDoll* doll);

    void removeRagDoll(RagDoll* doll);

    /// \return distance of largest movement
    float enforceConstraints();

    /// \return number of collisions
    int computeCollisions();

    void processCollisions();

private:
    CollisionList _collisionList;
    QVector<RagDoll*> _dolls;
};

#endif // hifi_SimulationEngine_h
