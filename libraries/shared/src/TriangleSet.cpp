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
    _isBalanced = false;

    _triangles.push_back(t);
    _bounds += t.v0;
    _bounds += t.v1;
    _bounds += t.v2;
}

void TriangleSet::clear() {
    _triangles.clear();
    _bounds.clear();
    _isBalanced = false;

    // delete the octree?
}

bool TriangleSet::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
    float& distance, BoxFace& face, glm::vec3& surfaceNormal, bool precision) {

    if (!_isBalanced) {
        balanceOctree();
    }

    int trianglesTouched = 0;
    auto result = _triangleOctree.findRayIntersection(origin, direction, distance, face, surfaceNormal, precision, trianglesTouched);
    qDebug() << "trianglesTouched :" << trianglesTouched << "out of:" << _triangleOctree._population;
    return result;
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

void TriangleSet::debugDump() {
    qDebug() << __FUNCTION__;
    qDebug() << "bounds:" << getBounds();
    qDebug() << "triangles:" << size() << "at top level....";
    qDebug() << "----- _triangleOctree -----";
    _triangleOctree.debugDump();
}

void TriangleSet::balanceOctree() {
    _triangleOctree.reset(_bounds, 0);

    // insert all the triangles

    for (int i = 0; i < _triangles.size(); i++) {
        _triangleOctree.insert(i);
    }

    // prune the empty cells
    _triangleOctree.prune();

    _isBalanced = true;
}



void InternalTriangleSet::insert(int triangleIndex) {
    auto& triangle = _allTriangles[triangleIndex];

    _triangleIndices.push_back(triangleIndex);

    _bounds += triangle.v0;
    _bounds += triangle.v1;
    _bounds += triangle.v2;
}

void InternalTriangleSet::clear() {
    _triangleIndices.clear();
    _bounds.clear();
}

// Determine of the given ray (origin/direction) in model space intersects with any triangles
// in the set. If an intersection occurs, the distance and surface normal will be provided.
bool InternalTriangleSet::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
            float& distance, BoxFace& face, glm::vec3& surfaceNormal, bool precision, int& trianglesTouched) {

    bool intersectedSomething = false;
    float boxDistance = std::numeric_limits<float>::max();
    float bestDistance = std::numeric_limits<float>::max();

    if (_bounds.findRayIntersection(origin, direction, boxDistance, face, surfaceNormal)) {
        if (precision) {
            for (const auto& triangleIndex : _triangleIndices) {
                const auto& triangle = _allTriangles[triangleIndex];
                float thisTriangleDistance;
                trianglesTouched++;
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

static const int MAX_DEPTH = 3; // for now
static const int MAX_CHILDREN = 8;

TriangleOctreeCell::TriangleOctreeCell(std::vector<Triangle>& allTriangles, const AABox& bounds, int depth) :
    _allTriangles(allTriangles),
    _triangleSet(allTriangles)
{
    reset(bounds, depth);
}

void TriangleOctreeCell::clear() {
    _triangleSet.clear();
    _population = 0;
}

void TriangleOctreeCell::prune() {
    // do nothing yet...
}

void TriangleOctreeCell::reset(const AABox& bounds, int depth) {
    clear();
    _triangleSet._bounds = bounds;
    _depth = depth;
    if (depth <= MAX_DEPTH) {
        int childDepth = depth + 1;
        _children.clear();
        for (int child = 0; child < MAX_CHILDREN; child++) {
            AABox childBounds = getBounds().getOctreeChild((AABox::OctreeChild)child);
            _children.push_back(TriangleOctreeCell(_allTriangles, childBounds, childDepth));
        }
    }
}

void TriangleOctreeCell::debugDump() {
    qDebug() << __FUNCTION__;
    qDebug() << "bounds:" << getBounds();
    qDebug() << "depth:" << _depth;
    //qDebug() << "triangleSet:" << _triangleSet.size() << "at this level";
    qDebug() << "population:" << _population << "this level or below";
    if (_depth < MAX_DEPTH) {
        int childNum = 0;
        for (auto& child : _children) {
            qDebug() << "child:" << childNum;
            child.debugDump();
            childNum++;
        }
    }
}

void TriangleOctreeCell::insert(int triangleIndex) {
    const Triangle& triangle = _allTriangles[triangleIndex];
    _population++;
    // if we're not yet at the max depth, then check which child the triangle fits in
    if (_depth < MAX_DEPTH) {
        for (auto& child : _children) {
            if (child.getBounds().contains(triangle)) {
                child.insert(triangleIndex);
                return;
            }
        }
    }
    // either we're at max depth, or the triangle doesn't fit in one of our
    // children and so we want to just record it here
    _triangleSet.insert(triangleIndex);
}

bool TriangleOctreeCell::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                float& distance, BoxFace& face, glm::vec3& surfaceNormal, bool precision, int& trianglesTouched) {

    if (_population < 1) {
        return false; // no triangles below here, so we can't intersect
    }

    float bestLocalDistance = std::numeric_limits<float>::max();
    BoxFace bestLocalFace;
    glm::vec3 bestLocalNormal;
    bool intersects = false;

    // if the ray intersects our bounding box, then continue
    if (getBounds().findRayIntersection(origin, direction, bestLocalDistance, bestLocalFace, bestLocalNormal)) {
        bestLocalDistance = std::numeric_limits<float>::max();

        float childDistance = std::numeric_limits<float>::max();
        BoxFace childFace;
        glm::vec3 childNormal;

        // if we're not yet at the max depth, then check which child the triangle fits in
        if (_depth < MAX_DEPTH) {
            for (auto& child : _children) {
                // check each child, if there's an intersection, it will return some distance that we need
                // to compare against the other results, because there might be multiple intersections and
                // we will always choose the best (shortest) intersection
                if (child.findRayIntersection(origin, direction, childDistance, childFace, childNormal, precision, trianglesTouched)) {
                    if (childDistance < bestLocalDistance) {
                        bestLocalDistance = childDistance;
                        bestLocalFace = childFace;
                        bestLocalNormal = childNormal;
                        intersects = true;
                    }
                }
            }
        }
        // also check our local triangle set
        if (_triangleSet.findRayIntersection(origin, direction, childDistance, childFace, childNormal, precision, trianglesTouched)) {
            if (childDistance < bestLocalDistance) {
                bestLocalDistance = childDistance;
                bestLocalFace = childFace;
                bestLocalNormal = childNormal;
                intersects = true;
            }
        }
    }
    if (intersects) {
        distance = bestLocalDistance;
        face = bestLocalFace;
        surfaceNormal = bestLocalNormal;
    }
    return intersects;
}
