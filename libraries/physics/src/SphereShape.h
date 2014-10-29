//
//  SphereShape.h
//  libraries/physics/src
//
//  Created by Andrew Meadows on 2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SphereShape_h
#define hifi_SphereShape_h

#include <SharedUtil.h>

#include "Shape.h"


class SphereShape : public Shape {
public:
    SphereShape() : Shape(SPHERE_SHAPE) {}

    SphereShape(float radius) : Shape(SPHERE_SHAPE) {
        _boundingRadius = radius;
    }

    SphereShape(float radius, const glm::vec3& position) : Shape(SPHERE_SHAPE, position) {
        _boundingRadius = radius;
    }

    virtual ~SphereShape() {}

    float getRadius() const { return _boundingRadius; }

    void setRadius(float radius) { _boundingRadius = radius; }

    bool findRayIntersection(RayIntersectionInfo& intersection) const;

    float getVolume() const { return 1.3333333333f * PI * _boundingRadius * _boundingRadius * _boundingRadius; }
};

inline QDebug operator<<(QDebug debug, const SphereShape& shape) {
    debug << "SphereShape[ (" 
            << "position: "
            << shape.getTranslation().x << ", " << shape.getTranslation().y << ", " << shape.getTranslation().z
            << "radius: "
            << shape.getRadius()
            << "]";

    return debug;
}

inline QDebug operator<<(QDebug debug, const SphereShape* shape) {
    debug << "SphereShape[ (" 
            << "center: "
            << shape->getTranslation().x << ", " << shape->getTranslation().y << ", " << shape->getTranslation().z
            << "radius: "
            << shape->getRadius()
            << "]";

    return debug;
}

#endif // hifi_SphereShape_h
