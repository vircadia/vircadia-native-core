//
//  RagDoll.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.05.30
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RagDoll.h"

#include "CapsuleShape.h"
#include "CollisionInfo.h"
#include "SharedUtil.h"
#include "SphereShape.h"

// ----------------------------------------------------------------------------
// FixedConstraint
// ----------------------------------------------------------------------------
FixedConstraint::FixedConstraint(glm::vec3* point, const glm::vec3& anchor ) : _point(point), _anchor(anchor) {
}

float FixedConstraint::enforce() {
    assert(_point != NULL);
    float distance = glm::distance(_anchor, *_point);
    *_point = _anchor;
    return distance;
}

void FixedConstraint::setPoint(glm::vec3* point) {
    _point = point;
}

void FixedConstraint::setAnchor(const glm::vec3& anchor) {
    _anchor = anchor;
}

// ----------------------------------------------------------------------------
// DistanceConstraint
// ----------------------------------------------------------------------------
DistanceConstraint::DistanceConstraint(glm::vec3* startPoint, glm::vec3* endPoint) : _distance(-1.0f) {
    _points[0] = startPoint;
    _points[1] = endPoint;
    _distance = glm::distance(*(_points[0]), *(_points[1]));
}

DistanceConstraint::DistanceConstraint(const DistanceConstraint& other) {
    _distance = other._distance;
    _points[0] = other._points[0];
    _points[1] = other._points[1];
}

void DistanceConstraint::setDistance(float distance) {
    _distance = fabsf(distance);
}

float DistanceConstraint::enforce() {
    float newDistance = glm::distance(*(_points[0]), *(_points[1]));
    glm::vec3 direction(0.0f, 1.0f, 0.0f);
    if (newDistance > EPSILON) {
        direction = (*(_points[0]) - *(_points[1])) / newDistance;
    }
    glm::vec3 center = 0.5f * (*(_points[0]) + *(_points[1]));
    *(_points[0]) = center + (0.5f * _distance) * direction;
    *(_points[1]) = center - (0.5f * _distance) * direction;
    return glm::abs(newDistance - _distance);
}

void DistanceConstraint::updateProxyShape(Shape* shape, const glm::quat& rotation, const glm::vec3& translation) const {
    if (!shape) {
        return;
    }
    switch (shape->getType()) {
        case Shape::SPHERE_SHAPE: {
            // sphere collides at endPoint
            SphereShape* sphere = static_cast<SphereShape*>(shape);
            sphere->setPosition(translation + rotation * (*_points[1]));
        }
        break;
        case Shape::CAPSULE_SHAPE: {
           // capsule collides from startPoint to endPoint
            CapsuleShape* capsule = static_cast<CapsuleShape*>(shape);
            capsule->setEndPoints(translation + rotation * (*_points[0]), translation + rotation * (*_points[1]));
        }
        break;
        default:
        break;
    }
}

// ----------------------------------------------------------------------------
// RagDoll
// ----------------------------------------------------------------------------

RagDoll::RagDoll() : _shapes(NULL) {
}

RagDoll::~RagDoll() {
    clear();
}
    
void RagDoll::init(QVector<Shape*>* shapes, const QVector<int>& parentIndices, const QVector<glm::vec3>& points) {
    clear();
    _shapes = shapes;
    const int numPoints = points.size();
    assert(numPoints == parentIndices.size());
    _points.reserve(numPoints);
    for (int i = 0; i < numPoints; ++i) {
        glm::vec3 position = points[i];
        _points.push_back(position);

        int parentIndex = parentIndices[i];
        assert(parentIndex < i && parentIndex >= -1);
        if (parentIndex == -1) {
            FixedConstraint* anchor = new FixedConstraint(&(_points[i]), glm::vec3(0.0f));
            _constraints.push_back(anchor);
        } else {
            DistanceConstraint* stick = new DistanceConstraint(&(_points[i]), &(_points[parentIndex]));
            _constraints.push_back(stick);
        }
    }
}

/// Delete all data.
void RagDoll::clear() {
    int numConstraints = _constraints.size();
    for (int i = 0; i < numConstraints; ++i) {
        delete _constraints[i];
    }
    _constraints.clear();
    _points.clear();
}

float RagDoll::enforceConstraints() {
    float maxDistance = 0.0f;
    const int numConstraints = _constraints.size();
    for (int i = 0; i < numConstraints; ++i) {
        DistanceConstraint* c = static_cast<DistanceConstraint*>(_constraints[i]);
        //maxDistance = glm::max(maxDistance, _constraints[i]->enforce());
        maxDistance = glm::max(maxDistance, c->enforce());
    }
    return maxDistance;
}

void RagDoll::updateShapes(const glm::quat& rotation, const glm::vec3& translation) const {
    if (!_shapes) {
        return;
    }
    int numShapes = _shapes->size();
    int numConstraints = _constraints.size();

    // NOTE: we assume a one-to-one relationship between shapes and constraints
    for (int i = 0; i < numShapes && i < numConstraints; ++i) {
        _constraints[i]->updateProxyShape((*_shapes)[i], rotation, translation);
    }
}
