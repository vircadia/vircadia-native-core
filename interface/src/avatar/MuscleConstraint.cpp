//
//  MuscleConstraint.cpp
//  interface/src/avatar
//
//  Created by Andrew Meadows 2014.07.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <SharedUtil.h>
#include <VerletPoint.h>

#include "MuscleConstraint.h"

const float DEFAULT_MUSCLE_STRENGTH = 0.5f * MAX_MUSCLE_STRENGTH;

MuscleConstraint::MuscleConstraint(VerletPoint* parent, VerletPoint* child) : _rootPoint(parent), _childPoint(child), 
        _parentIndex(-1), _childndex(-1), _strength(DEFAULT_MUSCLE_STRENGTH) {
    _childOffset = child->_position - parent->_position;
}

float MuscleConstraint::enforce() {
    _childPoint->_position += _strength * (_rootPoint->_position + _childOffset - _childPoint->_position);
    return 0.0f;
}

void MuscleConstraint::setIndices(int parent, int child) {
    _parentIndex = parent;
    _childndex = child;
}

