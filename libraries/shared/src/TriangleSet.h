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

#pragma once

#include <vector>
#include <memory>

#include "AABox.h"
#include "GeometryUtil.h"

class TriangleSet {

    class TriangleTreeCell {
    public:
        TriangleTreeCell(std::vector<Triangle>& allTriangles) : _allTriangles(allTriangles) {}
        TriangleTreeCell(std::vector<Triangle>& allTriangles, const AABox& bounds, int depth);

        void insert(size_t triangleIndex);
        void reset(const AABox& bounds, int depth = 0);
        void clear();

        bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& invDirection,
            float& distance, BoxFace& face, Triangle& triangle, bool precision, int& trianglesTouched,
            bool allowBackface = false);
        bool findParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
            float& parabolicDistance, BoxFace& face, Triangle& triangle, bool precision, int& trianglesTouched,
            bool allowBackface = false);

        const AABox& getBounds() const { return _bounds; }

        void debugDump();

    protected:
        // checks our internal list of triangles
        bool findRayIntersectionInternal(const glm::vec3& origin, const glm::vec3& direction,
            float& distance, BoxFace& face, Triangle& triangle, bool precision, int& trianglesTouched,
            bool allowBackface = false);
        bool findParabolaIntersectionInternal(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
            float& parabolicDistance, BoxFace& face, Triangle& triangle, bool precision, int& trianglesTouched,
            bool allowBackface = false);

        std::pair<AABox, AABox> getTriangleTreeCellChildBounds();

        std::vector<Triangle>& _allTriangles;
        std::pair<std::shared_ptr<TriangleTreeCell>, std::shared_ptr<TriangleTreeCell>> _children;
        int _depth { 0 };
        int _population { 0 };
        AABox _bounds;
        std::vector<size_t> _triangleIndices;

        friend class TriangleSet;
    };

    using SortedTriangleCell = std::pair<float, std::shared_ptr<TriangleTreeCell>>;

public:
    TriangleSet() : _triangleTree(_triangles) {}

    void debugDump();

    void insert(const Triangle& t);

    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& invDirection,
        float& distance, BoxFace& face, Triangle& triangle, bool precision, bool allowBackface = false);
    bool findParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
        float& parabolicDistance, BoxFace& face, Triangle& triangle, bool precision, bool allowBackface = false);

    void balanceTree();

    void reserve(size_t size) { _triangles.reserve(size); } // reserve space in the datastructure for size number of triangles
    size_t size() const { return _triangles.size(); }
    void clear();

    // Determine if a point is "inside" all the triangles of a convex hull. It is the responsibility of the caller to
    // determine that the triangle set is indeed a convex hull. If the triangles added to this set are not in fact a 
    // convex hull, the result of this method is meaningless and undetermined.
    bool convexHullContains(const glm::vec3& point) const;
    const AABox& getBounds() const { return _bounds; }

protected:
    bool _isBalanced { false };
    std::vector<Triangle> _triangles;
    TriangleTreeCell _triangleTree;
    AABox _bounds;
};
