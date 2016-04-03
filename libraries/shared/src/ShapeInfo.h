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

enum ShapeType {
    SHAPE_TYPE_NONE,
    SHAPE_TYPE_BOX,
    SHAPE_TYPE_SPHERE,
    SHAPE_TYPE_ELLIPSOID,
    SHAPE_TYPE_PLANE,
    SHAPE_TYPE_COMPOUND,
    SHAPE_TYPE_CAPSULE_X,
    SHAPE_TYPE_CAPSULE_Y,
    SHAPE_TYPE_CAPSULE_Z,
    SHAPE_TYPE_CYLINDER_X,
    SHAPE_TYPE_CYLINDER_Y,
    SHAPE_TYPE_CYLINDER_Z,
    SHAPE_TYPE_LINE
};

class ShapeInfo {

public:
    void clear();

    void setParams(ShapeType type, const glm::vec3& halfExtents, QString url="");
    void setBox(const glm::vec3& halfExtents);
    void setSphere(float radius);
    void setEllipsoid(const glm::vec3& halfExtents);
    void setConvexHulls(const QVector<QVector<glm::vec3>>& points);
    void setCapsuleY(float radius, float halfHeight);
    void setOffset(const glm::vec3& offset);

    int getType() const { return _type; }

    const glm::vec3& getHalfExtents() const { return _halfExtents; }
    const glm::vec3& getOffset() const { return _offset; }

    const QVector<QVector<glm::vec3>>& getPoints() const { return _points; }
    uint32_t getNumSubShapes() const;

    void clearPoints () { _points.clear(); }
    void appendToPoints (const QVector<glm::vec3>& newPoints) { _points << newPoints; }

    float computeVolume() const;

    /// Returns whether point is inside the shape
    /// For compound shapes it will only return whether it is inside the bounding box
    bool contains(const glm::vec3& point) const;

    const DoubleHashKey& getHash() const;

protected:
    ShapeType _type = SHAPE_TYPE_NONE;
    glm::vec3 _halfExtents = glm::vec3(0.0f);
    glm::vec3 _offset = glm::vec3(0.0f);
    DoubleHashKey _doubleHashKey;
    QVector<QVector<glm::vec3>> _points; // points for convex collision hulls
    QUrl _url; // url for model of convex collision hulls
};

#endif // hifi_ShapeInfo_h

