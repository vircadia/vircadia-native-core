//
//  TriangleSet.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 3/2/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <vector>

#include "AABox.h"
#include "GeometryUtil.h"

class InternalTriangleSet {
public:
    InternalTriangleSet() { }

    void reserve(size_t size) { _triangles.reserve(size); } // reserve space in the datastructure for size number of triangles
    size_t size() const { return _triangles.size(); } 

    virtual void insert(const Triangle& t);
    void clear();

    // Determine if the given ray (origin/direction) in model space intersects with any triangles in the set. If an 
    // intersection occurs, the distance and surface normal will be provided.
    // note: this might side-effect internal structures
    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, 
        float& distance, BoxFace& face, glm::vec3& surfaceNormal, bool precision, int& trianglesTouched);

    // Determine if a point is "inside" all the triangles of a convex hull. It is the responsibility of the caller to
    // determine that the triangle set is indeed a convex hull. If the triangles added to this set are not in fact a 
    // convex hull, the result of this method is meaningless and undetermined.
    bool convexHullContains(const glm::vec3& point) const;
    const AABox& getBounds() const { return _bounds; }

protected:
    std::vector<Triangle> _triangles;
    AABox _bounds;

    friend class TriangleOctreeCell;
};

class TriangleOctreeCell {
public:
    TriangleOctreeCell() { }

    void insert(const Triangle& t);
    void reset(const AABox& bounds, int depth = 0);
    void clear();

    // Determine if the given ray (origin/direction) in model space intersects with any triangles in the set. If an 
    // intersection occurs, the distance and surface normal will be provided.
    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
        float& distance, BoxFace& face, glm::vec3& surfaceNormal, bool precision, int& trianglesTouched);

    const AABox& getBounds() const { return _triangleSet.getBounds(); }

    void debugDump();

protected:
    TriangleOctreeCell(const AABox& bounds, int depth);

    InternalTriangleSet _triangleSet;
    std::vector<TriangleOctreeCell> _children;
    int _depth { 0 };
    int _population { 0 };

    friend class TriangleSet;
};

class TriangleSet : public InternalTriangleSet {
    // pass through public implementation all the features of InternalTriangleSet
public:
    TriangleSet() { }

    void debugDump();

    virtual void insert(const Triangle& t) {
        _isBalanced = false;
        InternalTriangleSet::insert(t);
    }

    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
        float& distance, BoxFace& face, glm::vec3& surfaceNormal, bool precision);

    void balanceOctree();

protected:

    bool _isBalanced { false };
    TriangleOctreeCell _triangleOctree;
};
