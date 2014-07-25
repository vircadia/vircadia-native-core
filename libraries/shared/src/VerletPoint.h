//
//  VerletPoint.h
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.07.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VerletPoint_h
#define hifi_VerletPoint_h

#include <glm/glm.hpp>

class VerletPoint {
public:
    VerletPoint() : _position(0.0f), _lastPosition(0.0f), _mass(1.0f), _accumulatedDelta(0.0f), _numDeltas(0) {}

    void initPosition(const glm::vec3& position) { _position = position; _lastPosition = position; }
    void integrateForward();
    void accumulateDelta(const glm::vec3& delta);
    void applyAccumulatedDelta();

    glm::vec3 getAccumulatedDelta() const { 
        return (_numDeltas > 0) ?  _accumulatedDelta / (float)_numDeltas : glm::vec3(0.0f);
    }

    glm::vec3 _position;
    glm::vec3 _lastPosition;
    float _mass;

private:
    glm::vec3 _accumulatedDelta;
    int _numDeltas;
};

#endif // hifi_VerletPoint_h
