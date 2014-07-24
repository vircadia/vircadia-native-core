//
//  VerletPoint.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.07.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "VerletPoint.h"

void VerletPoint::accumulateDelta(const glm::vec3& delta) {
    _accumulatedDelta += delta;
    ++_numDeltas;
}

void VerletPoint::applyAccumulatedDelta() {
    if (_numDeltas > 0) { 
        _position += _accumulatedDelta / (float)_numDeltas;
        _accumulatedDelta = glm::vec3(0.0f);
        _numDeltas = 0;
    }
}
