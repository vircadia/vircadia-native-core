//
//  FixedConstraint.h
//  libraries/physics/src
//
//  Created by Andrew Meadows 2014.07.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FixedConstraint_h
#define hifi_FixedConstraint_h

#include <glm/glm.hpp>

#include "Constraint.h"

class VerletPoint;

// FixedConstraint takes pointers to a glm::vec3 and a VerletPoint.  
// The enforcement will copy the value of the vec3 into the VerletPoint.

class FixedConstraint : public Constraint {
public:
    FixedConstraint(glm::vec3* anchor, VerletPoint* point);
    ~FixedConstraint() {}
    float enforce();
    void setAnchor(glm::vec3* anchor);
    void setPoint(VerletPoint* point);
private:
    glm::vec3* _anchor;
    VerletPoint* _point;
};

#endif // hifi_FixedConstraint_h
