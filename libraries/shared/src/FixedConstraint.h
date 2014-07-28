//
//  FixedConstraint.h
//  libraries/shared/src
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

class FixedConstraint : public Constraint {
public:
    FixedConstraint(VerletPoint* point, const glm::vec3& anchor);
    float enforce();
    void setPoint(VerletPoint* point);
    void setAnchor(const glm::vec3& anchor);
private:
    VerletPoint* _point;
    glm::vec3 _anchor;
};

#endif // hifi_FixedConstraint_h
