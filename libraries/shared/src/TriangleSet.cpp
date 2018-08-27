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

#include "TriangleSet.h"

#include "GLMHelpers.h"

#include <list>

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

    _triangleTree.clear();
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
    qDebug() << "----- _triangleTree -----";
    _triangleTree.debugDump();
}

void TriangleSet::balanceTree() {
    _triangleTree.reset(_bounds);

    // insert all the triangles
    for (size_t i = 0; i < _triangles.size(); i++) {
        _triangleTree.insert(i);
    }

    _isBalanced = true;

#if WANT_DEBUGGING
    debugDump();
#endif
}

// With an octree: 8 ^ MAX_DEPTH = 4096 leaves
//static const int MAX_DEPTH = 4;
// With a k-d tree: 2 ^ MAX_DEPTH = 4096 leaves
static const int MAX_DEPTH = 12;

TriangleSet::TriangleTreeCell::TriangleTreeCell(std::vector<Triangle>& allTriangles, const AABox& bounds, int depth) :
    _allTriangles(allTriangles)
{
    reset(bounds, depth);
}

void TriangleSet::TriangleTreeCell::clear() {
    _population = 0;
    _triangleIndices.clear();
    _bounds.clear();
    _children.first.reset();
    _children.second.reset();
}

void TriangleSet::TriangleTreeCell::reset(const AABox& bounds, int depth) {
    clear();
    _bounds = bounds;
    _depth = depth;
}

void TriangleSet::TriangleTreeCell::debugDump() {
    qDebug() << __FUNCTION__;
    qDebug() << "               bounds:" << getBounds();
    qDebug() << "                depth:" << _depth;
    qDebug() << "           population:" << _population << "this level or below"
             << " ---- triangleIndices:" << _triangleIndices.size() << "in this cell";

    int numChildren = 0;
    if (_children.first) {
        numChildren++;
    } else if (_children.second) {
        numChildren++;
    }
    qDebug() << "child cells:" << numChildren;
    if (_depth < MAX_DEPTH) {
        if (_children.first) {
            qDebug() << "child: 0";
            _children.first->debugDump();
        }
        if (_children.second) {
            qDebug() << "child: 1";
            _children.second->debugDump();
        }
    }
}

std::pair<AABox, AABox> TriangleSet::TriangleTreeCell::getTriangleTreeCellChildBounds() {
    std::pair<AABox, AABox> toReturn;
    int axis = 0;
    // find biggest axis
    glm::vec3 dimensions = _bounds.getDimensions();
    for (int i = 0; i < 3; i++) {
        if (dimensions[i] >= dimensions[(i + 1) % 3] && dimensions[i] >= dimensions[(i + 2) % 3]) {
            axis = i;
            break;
        }
    }

    // The new boxes are half the size in the largest dimension
    glm::vec3 newDimensions = dimensions;
    newDimensions[axis] *= 0.5f;
    toReturn.first.setBox(_bounds.getCorner(), newDimensions);
    glm::vec3 offset = glm::vec3(0.0f);
    offset[axis] = newDimensions[axis];
    toReturn.second.setBox(_bounds.getCorner() + offset, newDimensions);
    return toReturn;
}

void TriangleSet::TriangleTreeCell::insert(size_t triangleIndex) {
    _population++;

    // if we're not yet at the max depth, then check which child the triangle fits in
    if (_depth < MAX_DEPTH) {
        const Triangle& triangle = _allTriangles[triangleIndex];
        auto childBounds = getTriangleTreeCellChildBounds();

        auto insertOperator = [&](const AABox& childBound, std::shared_ptr<TriangleTreeCell>& child) {
            // if the child AABox would contain the triangle...
            if (childBound.contains(triangle)) {
                // if the child cell doesn't yet exist, create it...
                if (!child) {
                    child = std::make_shared<TriangleTreeCell>(_allTriangles, childBound, _depth + 1);
                }

                // insert the triangleIndex in the child cell
                child->insert(triangleIndex);
                return true;
            }
            return false;
        };
        if (insertOperator(childBounds.first, _children.first) || insertOperator(childBounds.second, _children.second)) {
            return;
        }
    }
    // either we're at max depth, or the triangle doesn't fit in one of our
    // children and so we want to just record it here
    _triangleIndices.push_back(triangleIndex);
}

bool TriangleSet::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& invDirection, float& distance,
                                      BoxFace& face, Triangle& triangle, bool precision, bool allowBackface) {
    if (!_isBalanced) {
        balanceTree();
    }

    float localDistance = distance;
    int trianglesTouched = 0;
    bool hit = _triangleTree.findRayIntersection(origin, direction, invDirection, localDistance, face, triangle, precision, trianglesTouched, allowBackface);
    if (hit) {
        distance = localDistance;
    }

#if WANT_DEBUGGING
    if (precision) {
        qDebug() << "trianglesTouched :" << trianglesTouched << "out of:" << _triangleTree._population << "_triangles.size:" << _triangles.size();
    }
#endif
    return hit;
}

// Determine of the given ray (origin/direction) in model space intersects with any triangles
// in the set. If an intersection occurs, the distance and surface normal will be provided.
bool TriangleSet::TriangleTreeCell::findRayIntersectionInternal(const glm::vec3& origin, const glm::vec3& direction,
                                                                  float& distance, BoxFace& face, Triangle& triangle, bool precision,
                                                                  int& trianglesTouched, bool allowBackface) {
    bool intersectedSomething = false;
    float bestDistance = FLT_MAX;
    Triangle bestTriangle;

    if (precision) {
        for (const auto& triangleIndex : _triangleIndices) {
            const auto& thisTriangle = _allTriangles[triangleIndex];
            float thisTriangleDistance;
            trianglesTouched++;
            if (findRayTriangleIntersection(origin, direction, thisTriangle, thisTriangleDistance, allowBackface)) {
                if (thisTriangleDistance < bestDistance) {
                    bestDistance = thisTriangleDistance;
                    bestTriangle = thisTriangle;
                    intersectedSomething = true;
                }
            }
        }
    } else {
        intersectedSomething = true;
        bestDistance = distance;
    }

    if (intersectedSomething) {
        distance = bestDistance;
        triangle = bestTriangle;
    }

    return intersectedSomething;
}

bool TriangleSet::TriangleTreeCell::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& invDirection,
                                                        float& distance, BoxFace& face, Triangle& triangle, bool precision, int& trianglesTouched,
                                                        bool allowBackface) {
    if (_population < 1) {
        return false; // no triangles below here, so we can't intersect
    }

    float bestLocalDistance = FLT_MAX;
    BoxFace bestLocalFace;
    Triangle bestLocalTriangle;
    bool intersects = false;

    // Check our local triangle set first
    // The distance passed in here is the distance to our bounding box.  If !precision, that distance is used
    {
        float internalDistance = distance;
        BoxFace internalFace;
        Triangle internalTriangle;
        if (findRayIntersectionInternal(origin, direction, internalDistance, internalFace, internalTriangle, precision, trianglesTouched, allowBackface)) {
            bestLocalDistance = internalDistance;
            bestLocalFace = internalFace;
            bestLocalTriangle = internalTriangle;
            intersects = true;
        }
    }

    // if we're not yet at the max depth, then check our children
    if (_depth < MAX_DEPTH) {
        std::list<SortedTriangleCell> sortedTriangleCells;
        auto sortingOperator = [&](std::shared_ptr<TriangleTreeCell>& child) {
            if (child) {
                float priority = FLT_MAX;
                if (child->getBounds().contains(origin)) {
                    priority = 0.0f;
                } else {
                    float childBoundDistance = FLT_MAX;
                    BoxFace childBoundFace;
                    glm::vec3 childBoundNormal;
                    if (child->getBounds().findRayIntersection(origin, direction, invDirection, childBoundDistance, childBoundFace, childBoundNormal)) {
                        // We only need to add this cell if it's closer than the local triangle set intersection (if there was one)
                        if (childBoundDistance < bestLocalDistance) {
                            priority = childBoundDistance;
                        }
                    }
                }

                if (priority < FLT_MAX) {
                    if (sortedTriangleCells.size() > 0 && priority < sortedTriangleCells.front().first) {
                        sortedTriangleCells.emplace_front(priority, child);
                    } else {
                        sortedTriangleCells.emplace_back(priority, child);
                    }
                }
            }
        };
        sortingOperator(_children.first);
        sortingOperator(_children.second);

        for (auto it = sortedTriangleCells.begin(); it != sortedTriangleCells.end(); ++it) {
            const SortedTriangleCell& sortedTriangleCell = *it;
            float childDistance = sortedTriangleCell.first;
            // We can exit once childDistance > bestLocalDistance
            if (childDistance > bestLocalDistance) {
                break;
            }
            // If we're inside the child cell and !precision, we need the actual distance to the cell bounds
            if (!precision && childDistance < EPSILON) {
                BoxFace childBoundFace;
                glm::vec3 childBoundNormal;
                sortedTriangleCell.second->getBounds().findRayIntersection(origin, direction, invDirection, childDistance, childBoundFace, childBoundNormal);
            }
            BoxFace childFace;
            Triangle childTriangle;
            if (sortedTriangleCell.second->findRayIntersection(origin, direction, invDirection, childDistance, childFace, childTriangle, precision, trianglesTouched)) {
                if (childDistance < bestLocalDistance) {
                    bestLocalDistance = childDistance;
                    bestLocalFace = childFace;
                    bestLocalTriangle = childTriangle;
                    intersects = true;
                    break;
                }
            }
        }
    }

    if (intersects) {
        distance = bestLocalDistance;
        face = bestLocalFace;
        triangle = bestLocalTriangle;
    }
    return intersects;
}

bool TriangleSet::findParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
                                           float& parabolicDistance, BoxFace& face, Triangle& triangle, bool precision, bool allowBackface) {
    if (!_isBalanced) {
        balanceTree();
    }

    float localDistance = parabolicDistance;
    int trianglesTouched = 0;
    bool hit = _triangleTree.findParabolaIntersection(origin, velocity, acceleration, localDistance, face, triangle, precision, trianglesTouched, allowBackface);
    if (hit) {
        parabolicDistance = localDistance;
    }

#if WANT_DEBUGGING
    if (precision) {
        qDebug() << "trianglesTouched :" << trianglesTouched << "out of:" << _triangleTree._population << "_triangles.size:" << _triangles.size();
    }
#endif
    return hit;
}

bool TriangleSet::TriangleTreeCell::findParabolaIntersectionInternal(const glm::vec3& origin, const glm::vec3& velocity,
                                                                       const glm::vec3& acceleration, float& parabolicDistance,
                                                                       BoxFace& face, Triangle& triangle, bool precision,
                                                                       int& trianglesTouched, bool allowBackface) {
    bool intersectedSomething = false;
    float bestDistance = FLT_MAX;
    Triangle bestTriangle;

    if (precision) {
        for (const auto& triangleIndex : _triangleIndices) {
            const auto& thisTriangle = _allTriangles[triangleIndex];
            float thisTriangleDistance;
            trianglesTouched++;
            if (findParabolaTriangleIntersection(origin, velocity, acceleration, thisTriangle, thisTriangleDistance, allowBackface)) {
                if (thisTriangleDistance < bestDistance) {
                    bestDistance = thisTriangleDistance;
                    bestTriangle = thisTriangle;
                    intersectedSomething = true;
                }
            }
        }
    } else {
        intersectedSomething = true;
        bestDistance = parabolicDistance;
    }

    if (intersectedSomething) {
        parabolicDistance = bestDistance;
        triangle = bestTriangle;
    }

    return intersectedSomething;
}

bool TriangleSet::TriangleTreeCell::findParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity,
                                                               const glm::vec3& acceleration, float& parabolicDistance,
                                                               BoxFace& face, Triangle& triangle, bool precision,
                                                               int& trianglesTouched, bool allowBackface) {
    if (_population < 1) {
        return false; // no triangles below here, so we can't intersect
    }

    float bestLocalDistance = FLT_MAX;
    BoxFace bestLocalFace;
    Triangle bestLocalTriangle;
    bool intersects = false;

    // Check our local triangle set first
    // The distance passed in here is the distance to our bounding box.  If !precision, that distance is used
    {
        float internalDistance = parabolicDistance;
        BoxFace internalFace;
        Triangle internalTriangle;
        if (findParabolaIntersectionInternal(origin, velocity, acceleration, internalDistance, internalFace, internalTriangle, precision, trianglesTouched, allowBackface)) {
            bestLocalDistance = internalDistance;
            bestLocalFace = internalFace;
            bestLocalTriangle = internalTriangle;
            intersects = true;
        }
    }

    // if we're not yet at the max depth, then check our children
    if (_depth < MAX_DEPTH) {
        std::list<SortedTriangleCell> sortedTriangleCells;
        auto sortingOperator = [&](std::shared_ptr<TriangleTreeCell>& child) {
            if (child) {
                float priority = FLT_MAX;
                if (child->getBounds().contains(origin)) {
                    priority = 0.0f;
                } else {
                    float childBoundDistance = FLT_MAX;
                    BoxFace childBoundFace;
                    glm::vec3 childBoundNormal;
                    if (child->getBounds().findParabolaIntersection(origin, velocity, acceleration, childBoundDistance, childBoundFace, childBoundNormal)) {
                        // We only need to add this cell if it's closer than the local triangle set intersection (if there was one)
                        if (childBoundDistance < bestLocalDistance) {
                            priority = childBoundDistance;
                        }
                    }
                }

                if (priority < FLT_MAX) {
                    if (sortedTriangleCells.size() > 0 && priority < sortedTriangleCells.front().first) {
                        sortedTriangleCells.emplace_front(priority, child);
                    } else {
                        sortedTriangleCells.emplace_back(priority, child);
                    }
                }
            }
        };
        sortingOperator(_children.first);
        sortingOperator(_children.second);

        for (auto it = sortedTriangleCells.begin(); it != sortedTriangleCells.end(); ++it) {
            const SortedTriangleCell& sortedTriangleCell = *it;
            float childDistance = sortedTriangleCell.first;
            // We can exit once childDistance > bestLocalDistance
            if (childDistance > bestLocalDistance) {
                break;
            }
            // If we're inside the child cell and !precision, we need the actual distance to the cell bounds
            if (!precision && childDistance < EPSILON) {
                BoxFace childBoundFace;
                glm::vec3 childBoundNormal;
                sortedTriangleCell.second->getBounds().findParabolaIntersection(origin, velocity, acceleration, childDistance, childBoundFace, childBoundNormal);
            }
            BoxFace childFace;
            Triangle childTriangle;
            if (sortedTriangleCell.second->findParabolaIntersection(origin, velocity, acceleration, childDistance, childFace, childTriangle, precision, trianglesTouched)) {
                if (childDistance < bestLocalDistance) {
                    bestLocalDistance = childDistance;
                    bestLocalFace = childFace;
                    bestLocalTriangle = childTriangle;
                    intersects = true;
                    break;
                }
            }
        }
    }

    if (intersects) {
        parabolicDistance = bestLocalDistance;
        face = bestLocalFace;
        triangle = bestLocalTriangle;
    }
    return intersects;
}