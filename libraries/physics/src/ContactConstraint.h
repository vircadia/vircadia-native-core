//
//  ContactConstraint.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.07.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ContactConstraint_h
#define hifi_ContactConstraint_h

#include <glm/glm.hpp>

#include "Constraint.h"
#include "VerletPoint.h"

class ContactConstraint : public Constraint {
public:
    ContactConstraint(VerletPoint* pointA, VerletPoint* pointB);

    float enforce();
    float enforceWithNormal(const glm::vec3& normal);

    glm::vec3 getTargetPointA() const { return _pointB->_position - _offset; }

    void setOffset(const glm::vec3& offset) { _offset = offset; }
    void setStrength(float strength) { _strength = glm::clamp(strength, 0.0f, 1.0f); }
    float getStrength() const { return _strength; }
private:
    VerletPoint* _pointA;
    VerletPoint* _pointB;
    glm::vec3 _offset;  // from pointA toward pointB
    float _strength;    // a value in range [0,1]
};

#endif // hifi_ContactConstraint_h
