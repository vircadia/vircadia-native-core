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
#include "VerletPoint.h"

FixedConstraint::FixedConstraint(glm::vec3* anchor, VerletPoint* point) : _anchor(anchor), _point(point) {
    assert(anchor);
    assert(point);
}

float FixedConstraint::enforce() {
    assert(_point != NULL);
    _point->_position = *_anchor;
    _point->_lastPosition = *_anchor;
    return 0.0f;
}

void FixedConstraint::setAnchor(glm::vec3* anchor) { 
    assert(anchor);
    _anchor = anchor; 
}

void FixedConstraint::setPoint(VerletPoint* point) {
    assert(point);
    _point = point;
}
