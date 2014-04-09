//
//  SphereShape.h
//  libraries/shared/src
//
//  Created by Andrew Meadows on 2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__SphereShape__
#define __hifi__SphereShape__

#include "Shape.h"

class SphereShape : public Shape {
public:
    SphereShape() : Shape(Shape::SPHERE_SHAPE) {}

    SphereShape(float radius) : Shape(Shape::SPHERE_SHAPE) {
        _boundingRadius = radius;
    }

    SphereShape(float radius, const glm::vec3& position) : Shape(Shape::SPHERE_SHAPE, position) {
        _boundingRadius = radius;
    }

    float getRadius() const { return _boundingRadius; }

    void setRadius(float radius) { _boundingRadius = radius; }
};

#endif /* defined(__hifi__SphereShape__) */
