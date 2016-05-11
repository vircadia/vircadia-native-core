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

#include "OctreeConstants.h"

class AABox;

/// renderAccuracy represents a floating point "visibility" of an object based on it's view from the camera. At a simple
/// level it returns 0.0f for things that are so small for the current settings that they could not be visible.
float calculateRenderAccuracy(const glm::vec3& position,
        const AABox& bounds,
        float octreeSizeScale = DEFAULT_OCTREE_SIZE_SCALE,
        int boundaryLevelAdjust = 0);

float boundaryDistanceForRenderLevel(unsigned int renderLevel, float voxelSizeScale);

float getAccuracyAngle(float octreeSizeScale, int boundaryLevelAdjust);

#endif // hifi_OctreeUtils_h
