//
//  TriangleSet.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 3/2/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLMHelpers.h"
#include "TriangleSet.h"

void TriangleSet::insert(const Triangle& t) {
    _triangles.push_back(t);

    _bounds += t.v0;
    _bounds += t.v1;
    _bounds += t.v2;
}

void TriangleSet::clear() {
    _triangles.clear();
    _bounds.clear();
}

// Determine of the given ray (origin/direction) in model space intersects with any triangles
// in the set. If an intersection occurs, the distance and surface normal will be provided.
bool TriangleSet::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                    float& distance, BoxFace& face, glm::vec3& surfaceNormal, bool precision) const {

    bool intersectedSomething = false;
    float boxDistance = std::numeric_limits<float>::max();
    float bestDistance = std::numeric_limits<float>::max();

    if (_bounds.findRayIntersection(origin, direction, boxDistance, face, surfaceNormal)) {
        if (precision) {
            for (const auto& triangle : _triangles) {
                float thisTriangleDistance;
                if (findRayTriangleIntersection(origin, direction, triangle, thisTriangleDistance)) {
                    if (thisTriangleDistance < bestDistance) {
                        bestDistance = thisTriangleDistance;
                        intersectedSomething = true;
                        surfaceNormal = triangle.getNormal();
                        distance = bestDistance;
                    }
                }
            }
        } else {
            intersectedSomething = true;
            distance = boxDistance;
        }
    }

    return intersectedSomething;
}


bool TriangleSet::convexHullContains(const glm::vec3& point) const {
    if (!_bounds.contains(point)) {
        return false;
    }

    bool insideMesh = true; // optimistic
    for (const auto& triangle : _triangles) {
        if (!isPointBehindTrianglesPlane(point, triangle.v0, triangle.v1, triangle.v2)) {
            // it's not behind at least one so we bail
            insideMesh = false;
            break;
        }

    }
    return insideMesh;
}

