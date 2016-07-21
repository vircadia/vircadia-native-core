//
//  ShapeInfo.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.10.29
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ShapeInfo_h
#define hifi_ShapeInfo_h

#include <QVector>
#include <QString>
#include <QUrl>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include "DoubleHashKey.h"

const float MIN_SHAPE_OFFSET = 0.001f; // offsets less than 1mm will be ignored

// Bullet has a mesh generation util for convex shapes that we used to
// trim convex hulls with many points down to only 42 points.
const int MAX_HULL_POINTS = 42;


const int32_t END_OF_MESH_PART = -1; // bogus vertex index at end of mesh part
const int32_t END_OF_MESH = -2; // bogus vertex index at end of mesh

enum ShapeType {
    SHAPE_TYPE_NONE,
    SHAPE_TYPE_BOX,
    SHAPE_TYPE_SPHERE,
    SHAPE_TYPE_CAPSULE_X,
    SHAPE_TYPE_CAPSULE_Y,
    SHAPE_TYPE_CAPSULE_Z,
    SHAPE_TYPE_CYLINDER_X,
    SHAPE_TYPE_CYLINDER_Y,
    SHAPE_TYPE_CYLINDER_Z,
    SHAPE_TYPE_HULL,
    SHAPE_TYPE_PLANE,
    SHAPE_TYPE_COMPOUND,
    SHAPE_TYPE_SIMPLE_HULL,
    SHAPE_TYPE_SIMPLE_COMPOUND,
    SHAPE_TYPE_STATIC_MESH
};

class ShapeInfo {

public:

    using PointList = QVector<glm::vec3>;
    using PointCollection = QVector<PointList>;
    using TriangleIndices = QVector<int32_t>;

    void clear();

    void setParams(ShapeType type, const glm::vec3& halfExtents, QString url="");
    void setBox(const glm::vec3& halfExtents);
    void setSphere(float radius);
    void setPointCollection(const PointCollection& pointCollection);
    void setCapsuleY(float radius, float halfHeight);
    void setOffset(const glm::vec3& offset);

    int getType() const { return _type; }

    const glm::vec3& getHalfExtents() const { return _halfExtents; }
    const glm::vec3& getOffset() const { return _offset; }
    uint32_t getNumSubShapes() const;

    PointCollection& getPointCollection() { return _pointCollection; }
    const PointCollection& getPointCollection() const { return _pointCollection; }

    TriangleIndices& getTriangleIndices() { return _triangleIndices; }
    const TriangleIndices& getTriangleIndices() const { return _triangleIndices; }

    int getLargestSubshapePointCount() const;

    float computeVolume() const;

    /// Returns whether point is inside the shape
    /// For compound shapes it will only return whether it is inside the bounding box
    bool contains(const glm::vec3& point) const;

    const DoubleHashKey& getHash() const;

protected:
    QUrl _url; // url for model of convex collision hulls
    PointCollection _pointCollection;
    TriangleIndices _triangleIndices;
    glm::vec3 _halfExtents = glm::vec3(0.0f);
    glm::vec3 _offset = glm::vec3(0.0f);
    mutable DoubleHashKey _doubleHashKey;
    ShapeType _type = SHAPE_TYPE_NONE;
};

#endif // hifi_ShapeInfo_h

