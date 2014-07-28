//
//  FixedConstraint.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.07.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FixedConstraint.h"
#include "Shape.h"  // for MAX_SHAPE_MASS
#include "VerletPoint.h"

FixedConstraint::FixedConstraint(VerletPoint* point, const glm::vec3& anchor) : _point(point), _anchor(anchor) {
}

float FixedConstraint::enforce() {
    assert(_point != NULL);
    // TODO: use fast approximate sqrt here
    float distance = glm::distance(_anchor, _point->_position);
    _point->_position = _anchor;
    return distance;
}

void FixedConstraint::setPoint(VerletPoint* point) {
    assert(point);
    _point = point;
    _point->_mass = MAX_SHAPE_MASS;
}

void FixedConstraint::setAnchor(const glm::vec3& anchor) {
    _anchor = anchor;
}
