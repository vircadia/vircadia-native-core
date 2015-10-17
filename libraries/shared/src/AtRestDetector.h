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
    AtRestDetector(const glm::vec3& startPosition, const glm::quat& startRotation);
    void reset(const glm::vec3& startPosition, const glm::quat& startRotation);

    // returns true if object is at rest, dt in assumed to be seconds.
    bool update(float dt, const glm::vec3& position, const glm::quat& startRotation);

protected:
    glm::vec3 _positionAverage;
    float _positionVariance;

    glm::vec3 _quatLogAverage;
    float _quatLogVariance;
};

#endif
