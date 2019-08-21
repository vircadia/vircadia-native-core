//
//  OctreeUtils.cpp
//  libraries/octree/src
//
//  Created by Andrew Meadows 2016.03.04
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OctreeUtils.h"

#include <mutex>

#include <glm/glm.hpp>

#include <AABox.h>
#include <AACube.h>

float boundaryDistanceForRenderLevel(unsigned int renderLevel, float visibilityDistance) {
    return visibilityDistance / powf(2.0f, renderLevel);
}

float getPerspectiveAccuracyHalfAngleTan(float visibilityDistance, int boundaryLevelAdjust) {
    float visibleDistanceAtMaxScale = boundaryDistanceForRenderLevel(boundaryLevelAdjust, visibilityDistance);
    return UNIT_ELEMENT_MAX_EXTENT / visibleDistanceAtMaxScale;
}

float getPerspectiveAccuracyHalfAngle(float visibilityDistance, int boundaryLevelAdjust) {
    return atan(getPerspectiveAccuracyHalfAngleTan(visibilityDistance, boundaryLevelAdjust));
}

float getVisibilityDistanceFromHalfAngle(float halfAngle) {
    float halfAngleTan = tan(halfAngle);
    return UNIT_ELEMENT_MAX_EXTENT / halfAngleTan;
}

float getHalfAngleFromVisibilityDistance(float visibilityDistance) {
    float halfAngleTan = UNIT_ELEMENT_MAX_EXTENT / visibilityDistance;
    return atan(halfAngleTan);
}

float getOrthographicAccuracySize(float visibilityDistance, int boundaryLevelAdjust) {
    // Smallest visible element is 1cm
    const float smallestSize = 0.01f;
    return (smallestSize * DEFAULT_VISIBILITY_DISTANCE_FOR_UNIT_ELEMENT) / boundaryDistanceForRenderLevel(boundaryLevelAdjust, visibilityDistance);
}
