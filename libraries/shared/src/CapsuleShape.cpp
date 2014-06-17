//
//  CapsuleShape.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows on 02/20/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>
#include <glm/gtx/vector_angle.hpp>

#include "CapsuleShape.h"

#include "GeometryUtil.h"
#include "SharedUtil.h" 


// default axis of CapsuleShape is Y-axis
const glm::vec3 localAxis(0.0f, 1.0f, 0.0f);

CapsuleShape::CapsuleShape() : Shape(Shape::CAPSULE_SHAPE), _radius(0.0f), _halfHeight(0.0f) {}

CapsuleShape::CapsuleShape(float radius, float halfHeight) : Shape(Shape::CAPSULE_SHAPE),
    _radius(radius), _halfHeight(halfHeight) {
    updateBoundingRadius();
}

CapsuleShape::CapsuleShape(float radius, float halfHeight, const glm::vec3& position, const glm::quat& rotation) : 
    Shape(Shape::CAPSULE_SHAPE, position, rotation), _radius(radius), _halfHeight(halfHeight) {
    updateBoundingRadius();
}

CapsuleShape::CapsuleShape(float radius, const glm::vec3& startPoint, const glm::vec3& endPoint) :
    Shape(Shape::CAPSULE_SHAPE), _radius(radius), _halfHeight(0.0f) {
    setEndPoints(startPoint, endPoint);
}

/// \param[out] startPoint is the center of start cap
void CapsuleShape::getStartPoint(glm::vec3& startPoint) const {
    startPoint = _position - _rotation * glm::vec3(0.0f, _halfHeight, 0.0f);
}

/// \param[out] endPoint is the center of the end cap
void CapsuleShape::getEndPoint(glm::vec3& endPoint) const {
    endPoint = _position + _rotation * glm::vec3(0.0f, _halfHeight, 0.0f);
}

void CapsuleShape::computeNormalizedAxis(glm::vec3& axis) const {
    // default axis of a capsule is along the yAxis
    axis = _rotation * glm::vec3(0.0f, 1.0f, 0.0f);
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

void CapsuleShape::setEndPoints(const glm::vec3& startPoint, const glm::vec3& endPoint) {
    glm::vec3 axis = endPoint - startPoint;
    _position = 0.5f * (endPoint + startPoint);
    float height = glm::length(axis);
    if (height > EPSILON) {
        _halfHeight = 0.5f * height;
        axis /= height;
        glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
        float angle = glm::angle(axis, yAxis);
        if (angle > EPSILON) {
            axis = glm::normalize(glm::cross(yAxis, axis));
            _rotation = glm::angleAxis(angle, axis);
        }
    }
    updateBoundingRadius();
}

bool CapsuleShape::findRayIntersection(const glm::vec3& rayStart, const glm::vec3& rayDirection, float& distance) const {
        glm::vec3 capsuleStart, capsuleEnd;
        getStartPoint(capsuleStart);
        getEndPoint(capsuleEnd);
        // NOTE: findRayCapsuleIntersection returns 'true' with distance = 0 when rayStart is inside capsule.
        // TODO: implement the raycast to return inside surface intersection for the internal rayStart.
        return findRayCapsuleIntersection(rayStart, rayDirection, capsuleStart, capsuleEnd, _radius, distance);
}
