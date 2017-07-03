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

class TriangleSet {

    class TriangleOctreeCell {
    public:
        TriangleOctreeCell(std::vector<Triangle>& allTriangles) :
            _allTriangles(allTriangles)
        { }

        void insert(size_t triangleIndex);
        void reset(const AABox& bounds, int depth = 0);
        void clear();

        bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
            float& distance, BoxFace& face, glm::vec3& surfaceNormal, bool precision, int& trianglesTouched, bool allowBackface = false);

        const AABox& getBounds() const { return _bounds; }

        void debugDump();

    protected:
        TriangleOctreeCell(std::vector<Triangle>& allTriangles, const AABox& bounds, int depth);

        // checks our internal list of triangles
        bool findRayIntersectionInternal(const glm::vec3& origin, const glm::vec3& direction,
            float& distance, BoxFace& face, glm::vec3& surfaceNormal, bool precision, int& trianglesTouched, bool allowBackface = false);

        std::vector<Triangle>& _allTriangles;
        std::map<AABox::OctreeChild, TriangleOctreeCell> _children;
        int _depth{ 0 };
        int _population{ 0 };
        AABox _bounds;
        std::vector<size_t> _triangleIndices;

        friend class TriangleSet;
    };

public:
    TriangleSet() :
        _triangleOctree(_triangles)
    {}

    void debugDump();

    void insert(const Triangle& t);

    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
        float& distance, BoxFace& face, glm::vec3& surfaceNormal, bool precision, bool allowBackface = false);

    void balanceOctree();

    void reserve(size_t size) { _triangles.reserve(size); } // reserve space in the datastructure for size number of triangles
    size_t size() const { return _triangles.size(); }
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

    bool _isBalanced{ false };
    TriangleOctreeCell _triangleOctree;
    std::vector<Triangle> _triangles;
    AABox _bounds;
};
