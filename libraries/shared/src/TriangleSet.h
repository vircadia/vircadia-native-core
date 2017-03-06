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

#include <QVector>

#include "AABox.h"
#include "GeometryUtil.h"

class TriangleSet {
public:
    void insertTriangle(const Triangle& t);
    void clear();

    // Determine of the given ray (origin/direction) in model space intersects with any triangles
    // in the set. If an intersection occurs, the distance and surface normal will be provided.
    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, 
        float& distance, BoxFace& face, glm::vec3& surfaceNormal, bool precision) const;

    bool convexHullContains(const glm::vec3& point) const; // this point is "inside" all triangles
    const AABox& getBounds() const { return _bounds; }

private:
    QVector<Triangle> _triangles;
    AABox _bounds;
};
