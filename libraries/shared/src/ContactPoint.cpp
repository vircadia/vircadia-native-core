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

ContactPoint::ContactPoint() : _lastFrame(0), _shapeA(NULL), _shapeB(NULL), 
        _offsetA(0.0f), _offsetB(0.0f), _normal(0.0f) {
}

ContactPoint::ContactPoint(const CollisionInfo& collision, quint32 frame) : _lastFrame(frame),
        _shapeA(collision.getShapeA()), _shapeB(collision.getShapeB()), _offsetA(0.0f), _offsetB(0.0f), 
        _numPointsA(0), _numPoints(0), _normal(0.0f) {

    _contactPoint = collision._contactPoint - 0.5f * collision._penetration;
    _offsetA = collision._contactPoint - _shapeA->getTranslation();
    _offsetB = collision._contactPoint - collision._penetration - _shapeB->getTranslation();
    float pLength = glm::length(collision._penetration);
    if (pLength > EPSILON) {
        _normal = collision._penetration / pLength;
    }

    if (_shapeA->getID() > _shapeB->getID()) {
        // swap so that _shapeA always has lower ID
        _shapeA = collision.getShapeB();
        _shapeB = collision.getShapeA();

        glm::vec3 temp = _offsetA;
        _offsetA = _offsetB;
        _offsetB = temp;
        _normal = - _normal;
    }

    _shapeA->getVerletPoints(_points);
    _numPointsA = _points.size();
    _shapeB->getVerletPoints(_points);
    _numPoints = _points.size();

    // compute offsets for shapeA
    for (int i = 0; i < _numPointsA; ++i) {
        glm::vec3 offset = _points[i]->_position - collision._contactPoint;
        _offsets.push_back(offset);
        _distances.push_back(glm::length(offset));
    }
    // compute offsets for shapeB
    for (int i = _numPointsA; i < _numPoints; ++i) {
        glm::vec3 offset = _points[i]->_position - collision._contactPoint + collision._penetration;
        _offsets.push_back(offset);
        _distances.push_back(glm::length(offset));
    }
}

// virtual 
float ContactPoint::enforce() {
    int numPoints = _points.size();
    for (int i = 0; i < numPoints; ++i) {
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
    return 0.0f;
}

void ContactPoint::buildConstraints() {
    glm::vec3 pointA = _shapeA->getTranslation() + _offsetA;
    glm::vec3 pointB = _shapeB->getTranslation() + _offsetB;
    glm::vec3 penetration = pointA - pointB;
    float pDotN = glm::dot(penetration, _normal);
    bool actuallyMovePoints = (pDotN > EPSILON);

    // the contact point will be the average of the two points on the shapes
    _contactPoint = 0.5f * (pointA + pointB);

    // TODO: Andrew to compute more correct lagrangian weights that provide a more realistic response.
    //
    // HACK: since the weights are naively equal for all points (which is what the above TODO is about) we 
    // don't want to use the full-strength delta because otherwise there can be annoying oscillations.  We 
    // reduce this problem by in the short-term by attenuating the delta that is applied, the tradeoff is
    // that this makes it easier for limbs to tunnel through during collisions.
    const float HACK_STRENGTH = 0.5f;

    int numPoints = _points.size();
    for (int i = 0; i < numPoints; ++i) {
        VerletPoint* point = _points[i];
        glm::vec3 offset = _offsets[i];

        // split delta into parallel and perpendicular components
        glm::vec3 delta = _contactPoint + offset - point->_position;
        glm::vec3 paraDelta = glm::dot(delta, _normal) * _normal;
        glm::vec3 perpDelta = delta - paraDelta;
        
        // use the relative sizes of the components to decide how much perpenducular delta to use
        // perpendicular < parallel ==> static friciton ==> perpFactor = 1.0
        // perpendicular > parallel ==> dynamic friciton ==> cap to length of paraDelta ==> perpFactor < 1.0
        float paraLength = glm::length(paraDelta);
        float perpLength = glm::length(perpDelta);
        float perpFactor = (perpLength > paraLength && perpLength > EPSILON) ? (paraLength / perpLength) : 1.0f;

        // recombine the two components to get the final delta
        delta = paraDelta + perpFactor * perpDelta;
    
        glm::vec3 targetPosition = point->_position + delta;
        _distances[i] = glm::distance(_contactPoint, targetPosition);
        if (actuallyMovePoints) {
            point->_position += HACK_STRENGTH * delta;
        }
    }
}

void ContactPoint::updateContact(const CollisionInfo& collision, quint32 frame) {
    _lastFrame = frame;
    _contactPoint = collision._contactPoint - 0.5f * collision._penetration;
    _offsetA = collision._contactPoint - collision._shapeA->getTranslation();
    _offsetB = collision._contactPoint - collision._penetration - collision._shapeB->getTranslation();
    float pLength = glm::length(collision._penetration);
    if (pLength > EPSILON) {
        _normal = collision._penetration / pLength;
    } else {
        _normal = glm::vec3(0.0f);
    }

    if (collision._shapeA->getID() > collision._shapeB->getID()) {
        // our _shapeA always has lower ID
        glm::vec3 temp = _offsetA;
        _offsetA = _offsetB;
        _offsetB = temp;
        _normal = - _normal;
    }

    // compute offsets for shapeA
    assert(_offsets.size() == _numPoints);
    for (int i = 0; i < _numPointsA; ++i) {
        _offsets[i] = (_points[i]->_position - collision._contactPoint);
    }
    // compute offsets for shapeB
    for (int i = _numPointsA; i < _numPoints; ++i) {
        _offsets[i] = (_points[i]->_position - collision._contactPoint + collision._penetration);
    }
}
