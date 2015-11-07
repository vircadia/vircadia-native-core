//
//  AtRestDetector.cpp
//  libraries/shared/src
//
//  Created by Anthony Thibault on 10/6/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AtRestDetector.h"

#include "NumericalConstants.h"
#include "SharedLogging.h"
#include "SharedUtil.h"

AtRestDetector::AtRestDetector(const glm::vec3& startPosition, const glm::quat& startRotation) {
    reset(startPosition, startRotation);
}

void AtRestDetector::reset(const glm::vec3& startPosition, const glm::quat& startRotation) {
    _positionAverage = startPosition;
    _positionVariance = 0.0f;

    glm::quat ql = glm::log(startRotation);
    _quatLogAverage = glm::vec3(ql.x, ql.y, ql.z);
    _quatLogVariance = 0.0f;
    _lastUpdateTime = usecTimestampNow();
    _isAtRest = false;
}

bool AtRestDetector::update(const glm::vec3& position, const glm::quat& rotation) {
    uint64_t now = usecTimestampNow();
    float dt = (float)(now - _lastUpdateTime) / (float)USECS_PER_SECOND;
    _lastUpdateTime = now;
    const float TAU = 1.0f;
    float delta = glm::min(dt / TAU, 1.0f);

    // keep a running average of position.
    _positionAverage = position * delta + _positionAverage * (1 - delta);

    // keep a running average of position variances.
    glm::vec3 dx = position - _positionAverage;
    _positionVariance = glm::dot(dx, dx) * delta + _positionVariance * (1 - delta);

    // keep a running average of quaternion logarithms.
    glm::quat quatLogAsQuat = glm::log(rotation);
    glm::vec3 quatLog(quatLogAsQuat.x, quatLogAsQuat.y, quatLogAsQuat.z);
    _quatLogAverage = quatLog * delta + _quatLogAverage * (1 - delta);

    // keep a running average of quatLog variances.
    glm::vec3 dql = quatLog - _quatLogAverage;
    _quatLogVariance = glm::dot(dql, dql) * delta + _quatLogVariance * (1 - delta);

    const float POSITION_VARIANCE_THRESHOLD = 0.001f;
    const float QUAT_LOG_VARIANCE_THRESHOLD = 0.00002f;

    _isAtRest = _positionVariance < POSITION_VARIANCE_THRESHOLD && _quatLogVariance < QUAT_LOG_VARIANCE_THRESHOLD;
    return _isAtRest;
}
