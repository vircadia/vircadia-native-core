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


float calculateRenderAccuracy(const glm::vec3& position,
        const AABox& bounds,
        float octreeSizeScale,
        int boundaryLevelAdjust) {
    float largestDimension = bounds.getLargestDimension();

    const float maxScale = (float)TREE_SCALE;
    float visibleDistanceAtMaxScale = boundaryDistanceForRenderLevel(boundaryLevelAdjust, octreeSizeScale) / OCTREE_TO_MESH_RATIO;

    static std::once_flag once;
    static QMap<float, float> shouldRenderTable;
    std::call_once(once, [&] {
        float SMALLEST_SCALE_IN_TABLE = 0.001f; // 1mm is plenty small
        float scale = maxScale;
        float factor = 1.0f;

        while (scale > SMALLEST_SCALE_IN_TABLE) {
            scale /= 2.0f;
            factor /= 2.0f;
            shouldRenderTable[scale] = factor;
        }
    });

    float closestScale = maxScale;
    float visibleDistanceAtClosestScale = visibleDistanceAtMaxScale;
    QMap<float, float>::const_iterator lowerBound = shouldRenderTable.lowerBound(largestDimension);
    if (lowerBound != shouldRenderTable.constEnd()) {
        closestScale = lowerBound.key();
        visibleDistanceAtClosestScale = visibleDistanceAtMaxScale * lowerBound.value();
    }

    if (closestScale < largestDimension) {
        visibleDistanceAtClosestScale *= 2.0f;
    }

    // FIXME - for now, it's either visible or not visible. We want to adjust this to eventually return
    // a floating point for objects that have small angular size to indicate that they may be rendered
    // with lower preciscion
    float distanceToCamera = glm::length(bounds.calcCenter() - position);
    return (distanceToCamera <= visibleDistanceAtClosestScale) ? 1.0f : 0.0f;
}

float boundaryDistanceForRenderLevel(unsigned int renderLevel, float voxelSizeScale) {
    return voxelSizeScale / powf(2.0f, renderLevel);
}

float getAccuracyAngle(float octreeSizeScale, int boundaryLevelAdjust) {
    const float maxScale = (float)TREE_SCALE;
    float visibleDistanceAtMaxScale = boundaryDistanceForRenderLevel(boundaryLevelAdjust, octreeSizeScale) / OCTREE_TO_MESH_RATIO;
    return atan(maxScale / visibleDistanceAtMaxScale);
}
