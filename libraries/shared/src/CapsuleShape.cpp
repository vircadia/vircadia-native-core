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


CapsuleShape::CapsuleShape() : Shape(CAPSULE_SHAPE), _radius(0.0f), _halfHeight(0.0f) {}

CapsuleShape::CapsuleShape(float radius, float halfHeight) : Shape(CAPSULE_SHAPE),
    _radius(radius), _halfHeight(halfHeight) {
    updateBoundingRadius();
}

CapsuleShape::CapsuleShape(float radius, float halfHeight, const glm::vec3& position, const glm::quat& rotation) : 
    Shape(CAPSULE_SHAPE, position, rotation), _radius(radius), _halfHeight(halfHeight) {
    updateBoundingRadius();
}

CapsuleShape::CapsuleShape(float radius, const glm::vec3& startPoint, const glm::vec3& endPoint) :
    Shape(CAPSULE_SHAPE), _radius(radius), _halfHeight(0.0f) {
    setEndPoints(startPoint, endPoint);
}

/// \param[out] startPoint is the center of start cap
void CapsuleShape::getStartPoint(glm::vec3& startPoint) const {
    startPoint = _translation - _rotation * glm::vec3(0.0f, _halfHeight, 0.0f);
}

/// \param[out] endPoint is the center of the end cap
void CapsuleShape::getEndPoint(glm::vec3& endPoint) const {
    endPoint = _translation + _rotation * glm::vec3(0.0f, _halfHeight, 0.0f);
}

void CapsuleShape::computeNormalizedAxis(glm::vec3& axis) const {
    // default axis of a capsule is along the yAxis
    axis = _rotation * DEFAULT_CAPSULE_AXIS;
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
    _translation = 0.5f * (endPoint + startPoint);
    float height = glm::length(axis);
    if (height > EPSILON) {
        _halfHeight = 0.5f * height;
        axis /= height;
        _rotation = computeNewRotation(axis);
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

// static
glm::quat CapsuleShape::computeNewRotation(const glm::vec3& newAxis) {
    float angle = glm::angle(newAxis, DEFAULT_CAPSULE_AXIS);
    if (angle > EPSILON) {
        glm::vec3 rotationAxis = glm::normalize(glm::cross(DEFAULT_CAPSULE_AXIS, newAxis));
        return glm::angleAxis(angle, rotationAxis);
    }
    return glm::quat();
}
