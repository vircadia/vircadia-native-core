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
#include <glm/glm.hpp>

#include "Shape.h"

enum ShapeType {
    SHAPE_TYPE_NONE,
    SHAPE_TYPE_BOX,
    SHAPE_TYPE_SPHERE,
    SHAPE_TYPE_ELLIPSOID,
    SHAPE_TYPE_CYLINDER,
    SHAPE_TYPE_CAPSULE,
    SHAPE_TYPE_CONVEX_HULL,
    SHAPE_TYPE_PLANE,
    SHAPE_TYPE_COMPOUND
};

class ShapeInfo {
public:
    ShapeInfo() : _type(SHAPE_TYPE_NONE) {}

    void clear();

    void setBox(const glm::vec3& halfExtents);
    void setSphere(float radius);
    void setCylinder(float radius, float halfHeight);
    void setCapsule(float radius, float halfHeight);

    const int getType() const { return _type; }
    const QVector<glm::vec3>& getData() const { return _data; }

    glm::vec3 getBoundingBoxDiagonal() const;
    float computeVolume() const;

protected:
    int _type;
    QVector<glm::vec3> _data;
};

#endif // hifi_ShapeInfo_h

