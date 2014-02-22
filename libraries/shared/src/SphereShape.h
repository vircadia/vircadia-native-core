//
//  SphereShape.h
//  hifi
//
//  Created by Andrew Meadows on 2014.02.20
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__SphereShape__
#define __hifi__SphereShape__

#include "Shape.h"

class SphereShape : public Shape {
public:
    SphereShape() : Shape(Shape::SPHERE_SHAPE) {}

    float getRadius() const { return _boundingRadius; }

    void setRadius(float radius) { _boundingRadius = radius; }
};

#endif /* defined(__hifi__SphereShape__) */
