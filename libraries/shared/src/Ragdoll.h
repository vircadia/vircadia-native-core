//
//  Ragdoll.h
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.05.30
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Ragdoll_h
#define hifi_Ragdoll_h

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "VerletPoint.h"

#include <QVector>

class Constraint;

class Ragdoll {
public:

    Ragdoll();
    virtual ~Ragdoll();

    virtual void stepRagdollForward(float deltaTime) = 0;

    /// \return max distance of point movement
    float enforceRagdollConstraints();

    // both const and non-const getPoints()
    const QVector<VerletPoint>& getRagdollPoints() const { return _ragdollPoints; }
    QVector<VerletPoint>& getRagdollPoints() { return _ragdollPoints; }

protected:
    void clearRagdollConstraintsAndPoints();
    virtual void initRagdollPoints() = 0;
    virtual void buildRagdollConstraints() = 0;

    QVector<VerletPoint> _ragdollPoints;
    QVector<Constraint*> _ragdollConstraints;
};

#endif // hifi_Ragdoll_h
