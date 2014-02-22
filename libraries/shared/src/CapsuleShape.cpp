//
//  CapsuleShape.cpp
//  hifi
//
//  Created by Andrew Meadows on 2014.02.20
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include <glm/gtx/vector_angle.hpp>

#include "CapsuleShape.h"
#include "SharedUtil.h" 


// default axis of CapsuleShape is Y-axis

CapsuleShape::CapsuleShape() : Shape(Shape::CAPSULE_SHAPE) {}

CapsuleShape::CapsuleShape(float radius, float halfHeight) : Shape(Shape::CAPSULE_SHAPE),
    _radius(radius), _halfHeight(halfHeight) {
    updateBoundingRadius();
}

CapsuleShape::CapsuleShape(float radius, float halfHeight, const glm::vec3& position, const glm::quat& rotation) : 
    Shape(Shape::CAPSULE_SHAPE, position, rotation), _radius(radius), _halfHeight(halfHeight) {
    updateBoundingRadius();
}

CapsuleShape::CapsuleShape(float radius, const glm::vec3& startPoint, const glm::vec3& endPoint) :
    Shape(Shape::CAPSULE_SHAPE), _radius(radius), _halfHeight(0.f) {
    glm::vec3 axis = endPoint - startPoint;
    float height = glm::length(axis);
    if (height > EPSILON) {
        _halfHeight = 0.5f * height;
        axis /= height;
        glm::vec3 yAxis(0.f, 1.f, 0.f);
        float angle = glm::angle(axis, yAxis);
        if (angle > EPSILON) {
            axis = glm::normalize(glm::cross(yAxis, axis));
            _rotation = glm::angleAxis(angle, axis);
        }
    }
    updateBoundingRadius();
}

void CapsuleShape::setRadius(float radius) {
    _radius = radius;
    updateBoundingRadius();
}

void CapsuleShape::setHalfHeight(float halfHeight) {
    _halfHeight = halfHeight;
    updateBoundingRadius();
}

void CapsuleShape::setRadiusAndHalfHeight(float radius, float halfHeight) {
    _radius = radius;
    _halfHeight = halfHeight;
    updateBoundingRadius();
}

