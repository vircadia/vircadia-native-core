//
//  ContactPoint.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.07.30
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ContactPoint.h"
#include "Shape.h"
#include "SharedUtil.h"

// This parameter helps keep the actual point of contact slightly inside each shape 
// which allows the collisions to happen almost every frame for more frequent updates.
const float CONTACT_PENETRATION_ALLOWANCE = 0.005f;

ContactPoint::ContactPoint() : 
        _lastFrame(0), _shapeA(NULL), _shapeB(NULL), 
        _offsetA(0.0f), _offsetB(0.0f), 
        _relativeMassA(0.5f), _relativeMassB(0.5f),
        _numPointsA(0), _numPoints(0), _normal(0.0f) {
}

ContactPoint::ContactPoint(const CollisionInfo& collision, quint32 frame) : 
        _lastFrame(frame), _shapeA(collision.getShapeA()), _shapeB(collision.getShapeB()), 
        _offsetA(0.0f), _offsetB(0.0f), 
        _relativeMassA(0.5f), _relativeMassB(0.5f), 
        _numPointsA(0), _numPoints(0), _normal(0.0f) {

    glm::vec3 pointA = collision._contactPoint;
    glm::vec3 pointB = collision._contactPoint - collision._penetration;

    float pLength = glm::length(collision._penetration);
    if (pLength > EPSILON) {
        _normal = collision._penetration / pLength;
    }
    if (_shapeA->getID() > _shapeB->getID()) {
        // swap so that _shapeA always has lower ID
        _shapeA = collision.getShapeB();
        _shapeB = collision.getShapeA();
        _normal = - _normal;
        pointA = pointB;
        pointB = collision._contactPoint;
    }

    // bring the contact points inside the shapes to help maintain collision updates
    pointA -= CONTACT_PENETRATION_ALLOWANCE * _normal;
    pointB += CONTACT_PENETRATION_ALLOWANCE * _normal;

    _offsetA = pointA - _shapeA->getTranslation();
    _offsetB = pointB - _shapeB->getTranslation();

    _shapeA->getVerletPoints(_points);
    _numPointsA = _points.size();
    _shapeB->getVerletPoints(_points);
    _numPoints = _points.size();

    // compute and cache relative masses
    float massA = EPSILON;
    for (int i = 0; i < _numPointsA; ++i) {
        massA += _points[i]->getMass();
    }
    float massB = EPSILON;
    for (int i = _numPointsA; i < _numPoints; ++i) {
        massB += _points[i]->getMass();
    }
    float invTotalMass = 1.0f / (massA + massB);
    _relativeMassA = massA * invTotalMass;
    _relativeMassB = massB * invTotalMass;

    // _contactPoint will be the weighted average of the two
    _contactPoint = _relativeMassA * pointA + _relativeMassB * pointB;

    // compute offsets for shapeA
    for (int i = 0; i < _numPointsA; ++i) {
        glm::vec3 offset = _points[i]->_position - pointA;
        _offsets.push_back(offset);
        _distances.push_back(glm::length(offset));
    }
    // compute offsets for shapeB
    for (int i = _numPointsA; i < _numPoints; ++i) {
        glm::vec3 offset = _points[i]->_position - pointB;
        _offsets.push_back(offset);
        _distances.push_back(glm::length(offset));
    }
}

float ContactPoint::enforce() {
    glm::vec3 pointA = _shapeA->getTranslation() + _offsetA;
    glm::vec3 pointB = _shapeB->getTranslation() + _offsetB;
    glm::vec3 penetration = pointA - pointB;
   float pDotN = glm::dot(penetration, _normal);
    bool constraintViolation = (pDotN > CONTACT_PENETRATION_ALLOWANCE);

    // the contact point will be the average of the two points on the shapes
    _contactPoint = _relativeMassA * pointA + _relativeMassB * pointB;

    if (constraintViolation) {
        for (int i = 0; i < _numPointsA; ++i) {
            VerletPoint* point = _points[i];
            glm::vec3 offset = _offsets[i];
    
            // split delta into parallel and perpendicular components
            glm::vec3 delta = _contactPoint + offset - point->_position;
            glm::vec3 paraDelta = glm::dot(delta, _normal) * _normal;
            glm::vec3 perpDelta = delta - paraDelta;
            
            // use the relative sizes of the components to decide how much perpenducular delta to use
            // perpendicular < parallel ==> static friction ==> perpFactor = 1.0
            // perpendicular > parallel ==> dynamic friction ==> cap to length of paraDelta ==> perpFactor < 1.0
            float paraLength = _relativeMassB * glm::length(paraDelta);
            float perpLength = _relativeMassA * glm::length(perpDelta);
            float perpFactor = (perpLength > paraLength && perpLength > EPSILON) ? (paraLength / perpLength) : 1.0f;
    
            // recombine the two components to get the final delta
            delta = paraDelta + perpFactor * perpDelta;
        
            glm::vec3 targetPosition = point->_position + delta;
            _distances[i] = glm::distance(_contactPoint, targetPosition);
            point->_position += delta;
        }
        for (int i = _numPointsA; i < _numPoints; ++i) {
            VerletPoint* point = _points[i];
            glm::vec3 offset = _offsets[i];
    
            // split delta into parallel and perpendicular components
            glm::vec3 delta = _contactPoint + offset - point->_position;
            glm::vec3 paraDelta = glm::dot(delta, _normal) * _normal;
            glm::vec3 perpDelta = delta - paraDelta;
            
            // use the relative sizes of the components to decide how much perpenducular delta to use
            // perpendicular < parallel ==> static friction ==> perpFactor = 1.0
            // perpendicular > parallel ==> dynamic friction ==> cap to length of paraDelta ==> perpFactor < 1.0
            float paraLength = _relativeMassA * glm::length(paraDelta);
            float perpLength = _relativeMassB * glm::length(perpDelta);
            float perpFactor = (perpLength > paraLength && perpLength > EPSILON) ? (paraLength / perpLength) : 1.0f;
    
            // recombine the two components to get the final delta
            delta = paraDelta + perpFactor * perpDelta;
        
            glm::vec3 targetPosition = point->_position + delta;
            _distances[i] = glm::distance(_contactPoint, targetPosition);
            point->_position += delta;
        }
    } else {
        for (int i = 0; i < _numPoints; ++i) {
            _distances[i] = glm::length(glm::length(_offsets[i]));
        }
    }
    return 0.0f;
}

// virtual 
void ContactPoint::applyFriction() {
    // TODO: Andrew to re-implement this in a different way
    /*
    for (int i = 0; i < _numPoints; ++i) {
        glm::vec3& position = _points[i]->_position;
        // TODO: use a fast distance approximation
        float newDistance = glm::distance(_contactPoint, position);
        float constrainedDistance = _distances[i];
        // NOTE: these "distance" constraints only push OUT, don't pull IN.
        if (newDistance > EPSILON && newDistance < constrainedDistance) {
            glm::vec3 direction = (_contactPoint - position) / newDistance;
            glm::vec3 center = 0.5f * (_contactPoint + position);
            _contactPoint = center + (0.5f * constrainedDistance) * direction;
            position = center - (0.5f * constrainedDistance) * direction;
        }
    }
    */
}

void ContactPoint::updateContact(const CollisionInfo& collision, quint32 frame) {
    _lastFrame = frame;

    // compute contact points on surface of each shape
    glm::vec3 pointA = collision._contactPoint;
    glm::vec3 pointB = pointA - collision._penetration;

    // compute the normal (which points from A into B)
    float pLength = glm::length(collision._penetration);
    if (pLength > EPSILON) {
        _normal = collision._penetration / pLength;
    } else {
        _normal = glm::vec3(0.0f);
    }

    if (collision._shapeA->getID() > collision._shapeB->getID()) {
        // our _shapeA always has lower ID
        _normal = - _normal;
        pointA = pointB;
        pointB = collision._contactPoint;
    }
    
    // bring the contact points inside the shapes to help maintain collision updates
    pointA -= CONTACT_PENETRATION_ALLOWANCE * _normal;
    pointB += CONTACT_PENETRATION_ALLOWANCE * _normal;

    // compute relative offsets to per-shape contact points
    _offsetA = pointA - collision._shapeA->getTranslation();
    _offsetB = pointB - collision._shapeB->getTranslation();

    // compute offsets for shapeA
    assert(_offsets.size() == _numPoints);
    for (int i = 0; i < _numPointsA; ++i) {
        _offsets[i] = _points[i]->_position - pointA;
    }
    // compute offsets for shapeB
    for (int i = _numPointsA; i < _numPoints; ++i) {
        _offsets[i] = _points[i]->_position - pointB;
    }
}
