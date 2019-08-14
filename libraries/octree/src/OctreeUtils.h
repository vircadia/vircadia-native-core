//
//  OctreeUtils.h
//  libraries/octree/src
//
//  Created by Andrew Meadows 2016.03.04
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeUtils_h
#define hifi_OctreeUtils_h

#include <NumericalConstants.h>

#include "OctreeConstants.h"

class AABox;
class AACube;
class QJsonDocument;

float boundaryDistanceForRenderLevel(unsigned int renderLevel, float visibilityDistance);

float getPerspectiveAccuracyHalfAngleTan(float visibilityDistance, int boundaryLevelAdjust);
float getPerspectiveAccuracyHalfAngle(float visibilityDistance, int boundaryLevelAdjust);
float getVisibilityDistanceFromHalfAngle(float halfAngle);
float getHalfAngleFromVisibilityDistance(float visibilityDistance);
float getOrthographicAccuracySize(float visibilityDistance, int boundaryLevelAdjust);

// MIN_ELEMENT_ANGULAR_DIAMETER = angular diameter of 1x1x1m cube at 400m = sqrt(3) / 400 = 0.0043301 radians ~= 0.25 degrees
const float MIN_ELEMENT_ANGULAR_DIAMETER = 0.0043301f; // radians
// NOTE: the entity bounding cube is larger than the smallest possible containing octree element by sqrt(3)
const float MIN_ENTITY_ANGULAR_DIAMETER = MIN_ELEMENT_ANGULAR_DIAMETER * SQRT_THREE;
const float MIN_VISIBLE_DISTANCE = 0.0001f; // helps avoid divide-by-zero check

#endif // hifi_OctreeUtils_h
