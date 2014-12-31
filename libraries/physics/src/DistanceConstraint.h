//
//  DistanceConstraint.h
//  libraries/physics/src
//
//  Created by Andrew Meadows 2014.07.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DistanceConstraint_h
#define hifi_DistanceConstraint_h

#include "Constraint.h"

class VerletPoint;

class DistanceConstraint : public Constraint {
public:
    DistanceConstraint(VerletPoint* startPoint, VerletPoint* endPoint);
    DistanceConstraint(const DistanceConstraint& other);
    float enforce();
    void setDistance(float distance);
    float getDistance() const { return _distance; }
private:
    float _distance;
    VerletPoint* _points[2];
};

#endif // hifi_DistanceConstraint_h
