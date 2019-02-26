//
//  AtRestDetector.h
//  libraries/shared/src
//
//  Created by Anthony Thibault on 10/6/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AtRestDetector_h
#define hifi_AtRestDetector_h

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

class AtRestDetector {
public:
    AtRestDetector() {};
    AtRestDetector(const glm::vec3& startPosition, const glm::quat& startRotation);
    void reset(const glm::vec3& startPosition, const glm::quat& startRotation);

    // returns true if object is at rest, dt in assumed to be seconds.
    void update(const glm::vec3& position, const glm::quat& startRotation);

    void invalidate() { _isValid = false; }
    bool isAtRest() const { return _isAtRest; }
    bool onRest() const { return !_lastIsAtRest && _isAtRest; }
    bool onWake() const { return _lastIsAtRest && !_isAtRest; }

protected:
    bool _isValid { false };
    glm::vec3 _positionAverage;
    glm::vec3 _quatLogAverage;
    uint64_t _lastUpdateTime { 0 };
    float _positionVariance { 0.0f };
    float _quatLogVariance { 0.0f };
    bool _isAtRest { false };
    bool _lastIsAtRest { false };
};

#endif
