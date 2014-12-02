//
//  CapsuleShape.cpp
//  libraries/physics/src
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

// helper
bool findRayIntersectionWithCap(const glm::vec3& sphereCenter, float sphereRadius, 
        const glm::vec3& capsuleCenter, RayIntersectionInfo& intersection) {
    float r2 = sphereRadius * sphereRadius;

    // compute closest approach (CA)
    float a = glm::dot(sphereCenter - intersection._rayStart, intersection._rayDirection); // a = distance from ray-start to CA
    float b2 = glm::distance2(sphereCenter, intersection._rayStart + a * intersection._rayDirection); // b2 = squared distance from sphere-center to CA
    if (b2 > r2) {
        // ray does not hit sphere
        return false;
    }
    float c = sqrtf(r2 - b2); // c = distance from CA to sphere surface along intersection._rayDirection
    float d2 = glm::distance2(intersection._rayStart, sphereCenter); // d2 = squared distance from sphere-center to ray-start
    float distance = FLT_MAX;
    if (a < 0.0f) {
        // ray points away from sphere-center
        if (d2 > r2) {
            // ray starts outside sphere
            return false;
        }
        // ray starts inside sphere
        distance = c + a;
    } else if (d2 > r2) {
        // ray starts outside sphere
        distance = a - c;
    } else {
        // ray starts inside sphere
        distance = a + c;
    }
    if (distance > 0.0f && distance < intersection._rayLength && distance < intersection._hitDistance) {
        glm::vec3 sphereCenterToHitPoint = intersection._rayStart + distance * intersection._rayDirection - sphereCenter;
        if (glm::dot(sphereCenterToHitPoint, sphereCenter - capsuleCenter) >= 0.0f) {
            intersection._hitDistance = distance;
            intersection._hitNormal = glm::normalize(sphereCenterToHitPoint);
            return true;
        }
    } 
    return false;
}

bool CapsuleShape::findRayIntersectionWithCaps(const glm::vec3& capsuleCenter, RayIntersectionInfo& intersection) const {
    glm::vec3 capCenter;
    getStartPoint(capCenter);
    bool hit = findRayIntersectionWithCap(capCenter, _radius, capsuleCenter, intersection);
    getEndPoint(capCenter);
    hit = findRayIntersectionWithCap(capCenter, _radius, capsuleCenter, intersection) || hit;
    if (hit) {
        intersection._hitShape = const_cast<CapsuleShape*>(this);
    }
    return hit;
}

bool CapsuleShape::findRayIntersection(RayIntersectionInfo& intersection) const {
    // ray is U, capsule is V
    glm::vec3 axisV;
    computeNormalizedAxis(axisV);
    glm::vec3 centerV = getTranslation();

    // first handle parallel case
    float uDotV = glm::dot(axisV, intersection._rayDirection);
    glm::vec3 UV = intersection._rayStart - centerV;
    if (glm::abs(1.0f - glm::abs(uDotV)) < EPSILON) {
        // line and cylinder are parallel
        float distanceV = glm::dot(UV, intersection._rayDirection);
        if (glm::length2(UV - distanceV * intersection._rayDirection) <= _radius * _radius) {
            // ray is inside cylinder's radius and might intersect caps
            return findRayIntersectionWithCaps(centerV, intersection);
        }
        return false;
    }
    
    // Given a line with point 'U' and normalized direction 'u' and 
    // a cylinder with axial point 'V', radius 'r', and normalized direction 'v'
    // the intersection of the two is on the line at distance 't' from 'U'.
    //
    // Determining the values of t reduces to solving a quadratic equation: At^2 + Bt + C = 0 
    //
    // where:
    //
    // UV = U-V
    // w = u-(u.v)v
    // Q = UV-(UV.v)v
    //       
    // A = w^2
    // B = 2(w.Q)
    // C = Q^2 - r^2

    glm::vec3 w = intersection._rayDirection - uDotV * axisV;
    glm::vec3 Q = UV - glm::dot(UV, axisV) * axisV;

    // we save a few multiplies by storing 2*A rather than just A
    float A2 = 2.0f * glm::dot(w, w);
    float B = 2.0f * glm::dot(w, Q);

    // since C is only ever used once (in the determinant) we compute it inline
    float determinant = B * B - 2.0f * A2 * (glm::dot(Q, Q) - _radius * _radius);
    if (determinant < 0.0f) {
        return false;
    }
    float hitLow  = (-B - sqrtf(determinant)) / A2;
    float hitHigh = -(hitLow + 2.0f * B / A2);

    if (hitLow > hitHigh) {
        // re-arrange so hitLow is always the smaller value
        float temp = hitHigh;
        hitHigh = hitLow;
        hitLow = temp;
    }
    if (hitLow < 0.0f) {
        if (hitHigh < 0.0f) {
            // capsule is completely behind rayStart
            return false;
        }
        hitLow = hitHigh;
    }

    glm::vec3 p = intersection._rayStart + hitLow * intersection._rayDirection;
    float d = glm::dot(p - centerV, axisV);
    if (glm::abs(d) <= getHalfHeight()) {
        // we definitely hit the cylinder wall
        intersection._hitDistance = hitLow;
        intersection._hitNormal = glm::normalize(p - centerV - d * axisV);
        intersection._hitShape = const_cast<CapsuleShape*>(this);
        return true;
    }

    // ray still might hit the caps
    return findRayIntersectionWithCaps(centerV, intersection);
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
