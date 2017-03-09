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
public:
    void reserve(size_t size) { _triangles.reserve(size); } // reserve space in the datastructure for size number of triangles
    size_t size() const { return _triangles.size(); } 

    const Triangle& getTriangle(size_t t) const { return _triangles[t]; }

    void insert(const Triangle& t);
    void clear();

    // Determine if the given ray (origin/direction) in model space intersects with any triangles in the set. If an 
    // intersection occurs, the distance and surface normal will be provided.
    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, 
        float& distance, BoxFace& face, glm::vec3& surfaceNormal, bool precision) const;

    // Determine if a point is "inside" all the triangles of a convex hull. It is the responsibility of the caller to
    // determine that the triangle set is indeed a convex hull. If the triangles added to this set are not in fact a 
    // convex hull, the result of this method is meaningless and undetermined.
    bool convexHullContains(const glm::vec3& point) const;
    const AABox& getBounds() const { return _bounds; }

private:
    std::vector<Triangle> _triangles;
    AABox _bounds;
};
