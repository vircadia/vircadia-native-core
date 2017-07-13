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

    _triangleOctree.clear();
}

bool TriangleSet::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
    float& distance, BoxFace& face, glm::vec3& surfaceNormal, bool precision, bool allowBackface) {

    // reset our distance to be the max possible, lower level tests will store best distance here
    distance = std::numeric_limits<float>::max();

    if (!_isBalanced) {
        balanceOctree();
    }

    int trianglesTouched = 0;
    auto result = _triangleOctree.findRayIntersection(origin, direction, distance, face, surfaceNormal, precision, trianglesTouched, allowBackface);

    #if WANT_DEBUGGING
    if (precision) {
        qDebug() << "trianglesTouched :" << trianglesTouched << "out of:" << _triangleOctree._population << "_triangles.size:" << _triangles.size();
    }
    #endif
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
    for (size_t i = 0; i < _triangles.size(); i++) {
        _triangleOctree.insert(i);
    }

    _isBalanced = true;

    #if WANT_DEBUGGING
    debugDump();
    #endif
}


// Determine of the given ray (origin/direction) in model space intersects with any triangles
// in the set. If an intersection occurs, the distance and surface normal will be provided.
bool TriangleSet::TriangleOctreeCell::findRayIntersectionInternal(const glm::vec3& origin, const glm::vec3& direction,
            float& distance, BoxFace& face, glm::vec3& surfaceNormal, bool precision, int& trianglesTouched, bool allowBackface) {

    bool intersectedSomething = false;
    float boxDistance = distance;
    float bestDistance = distance;

    if (_bounds.findRayIntersection(origin, direction, boxDistance, face, surfaceNormal)) {

        // if our bounding box intersects at a distance greater than the current known
        // best distance, than we can safely not check any of our triangles
        if (boxDistance > bestDistance) {
            return false;
        }

        if (precision) {
            for (const auto& triangleIndex : _triangleIndices) {
                const auto& triangle = _allTriangles[triangleIndex];
                float thisTriangleDistance;
                trianglesTouched++;
                if (findRayTriangleIntersection(origin, direction, triangle, thisTriangleDistance, allowBackface)) {
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

static const int MAX_DEPTH = 4; // for now
static const int MAX_CHILDREN = 8;

TriangleSet::TriangleOctreeCell::TriangleOctreeCell(std::vector<Triangle>& allTriangles, const AABox& bounds, int depth) :
    _allTriangles(allTriangles)
{
    reset(bounds, depth);
}

void TriangleSet::TriangleOctreeCell::clear() {
    _population = 0;
    _triangleIndices.clear();
    _bounds.clear();
    _children.clear();
}

void TriangleSet::TriangleOctreeCell::reset(const AABox& bounds, int depth) {
    clear();
    _bounds = bounds;
    _depth = depth;
}

void TriangleSet::TriangleOctreeCell::debugDump() {
    qDebug() << __FUNCTION__;
    qDebug() << "bounds:" << getBounds();
    qDebug() << "depth:" << _depth;
    qDebug() << "population:" << _population << "this level or below" 
             << " ---- triangleIndices:" << _triangleIndices.size() << "in this cell";

    qDebug() << "child cells:" << _children.size();
    if (_depth < MAX_DEPTH) {
        int childNum = 0;
        for (auto& child : _children) {
            qDebug() << "child:" << childNum;
            child.second.debugDump();
            childNum++;
        }
    }
}

void TriangleSet::TriangleOctreeCell::insert(size_t triangleIndex) {
    const Triangle& triangle = _allTriangles[triangleIndex];
    _population++;
    // if we're not yet at the max depth, then check which child the triangle fits in
    if (_depth < MAX_DEPTH) {

        for (int child = 0; child < MAX_CHILDREN; child++) {
            AABox childBounds = getBounds().getOctreeChild((AABox::OctreeChild)child);


            // if the child AABox would contain the triangle...
            if (childBounds.contains(triangle)) {
                // if the child cell doesn't yet exist, create it...
                if (_children.find((AABox::OctreeChild)child) == _children.end()) {
                    _children.insert(
                        std::pair<AABox::OctreeChild, TriangleOctreeCell>
                        ((AABox::OctreeChild)child, TriangleOctreeCell(_allTriangles, childBounds, _depth + 1)));
                }

                // insert the triangleIndex in the child cell
                _children.at((AABox::OctreeChild)child).insert(triangleIndex);
                return;
            }
        }
    }
    // either we're at max depth, or the triangle doesn't fit in one of our
    // children and so we want to just record it here
    _triangleIndices.push_back(triangleIndex);
}

bool TriangleSet::TriangleOctreeCell::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                float& distance, BoxFace& face, glm::vec3& surfaceNormal, bool precision, int& trianglesTouched, bool allowBackface) {

    if (_population < 1) {
        return false; // no triangles below here, so we can't intersect
    }

    float bestLocalDistance = distance;
    BoxFace bestLocalFace;
    glm::vec3 bestLocalNormal;
    bool intersects = false;

    // if the ray intersects our bounding box, then continue
    if (getBounds().findRayIntersection(origin, direction, bestLocalDistance, bestLocalFace, bestLocalNormal)) {

        // if the intersection with our bounding box, is greater than the current best distance (the distance passed in)
        // then we know that none of our triangles can represent a better intersection and we can return

        if (bestLocalDistance > distance) {
            return false;
        }

        bestLocalDistance = distance;

        float childDistance = distance;
        BoxFace childFace;
        glm::vec3 childNormal;

        // if we're not yet at the max depth, then check which child the triangle fits in
        if (_depth < MAX_DEPTH) {
            for (auto& child : _children) {
                // check each child, if there's an intersection, it will return some distance that we need
                // to compare against the other results, because there might be multiple intersections and
                // we will always choose the best (shortest) intersection
                if (child.second.findRayIntersection(origin, direction, childDistance, childFace, childNormal, precision, trianglesTouched)) {
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
        if (findRayIntersectionInternal(origin, direction, childDistance, childFace, childNormal, precision, trianglesTouched, allowBackface)) {
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
