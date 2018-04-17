//
//  EntityPriorityQueue.cpp
//  assignment-client/src/entities
//
//  Created by Andrew Meadows 2017.08.08
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityPriorityQueue.h"

const float PrioritizedEntity::DO_NOT_SEND = -1.0e-6f;
const float PrioritizedEntity::FORCE_REMOVE = -1.0e-5f;
const float PrioritizedEntity::WHEN_IN_DOUBT_PRIORITY = 1.0f;

void ConicalViewFrustum::set(const ViewFrustum& viewFrustum) {
    // The ConicalView has two parts: a central sphere (same as ViewFrustum) and a circular cone that bounds the frustum part.
    // Why?  Because approximate intersection tests are much faster to compute for a cone than for a frustum.
    _position = viewFrustum.getPosition();
    _direction = viewFrustum.getDirection();

    // We cache the sin and cos of the half angle of the cone that bounds the frustum.
    // (the math here is left as an exercise for the reader)
    float A = viewFrustum.getAspectRatio();
    float t = tanf(0.5f * viewFrustum.getFieldOfView());
    _cosAngle = 1.0f / sqrtf(1.0f + (A * A + 1.0f) * (t * t));
    _sinAngle = sqrtf(1.0f - _cosAngle * _cosAngle);

    _radius = viewFrustum.getCenterRadius();
}

float ConicalViewFrustum::computePriority(const AACube& cube) const {
    glm::vec3 p = cube.calcCenter() - _position; // position of bounding sphere in view-frame
    float d = glm::length(p); // distance to center of bounding sphere
    float r = 0.5f * cube.getScale(); // radius of bounding sphere
    if (d < _radius + r) {
        return r;
    }
    // We check the angle between the center of the cube and the _direction of the view.
    // If it is less than the sum of the half-angle from center of cone to outer edge plus
    // the half apparent angle of the bounding sphere then it is in view.
    //
    // The math here is left as an exercise for the reader with the following hints:
    // (1) We actually check the dot product of the cube's local position rather than the angle and
    // (2) we take advantage of this trig identity: cos(A+B) = cos(A)*cos(B) - sin(A)*sin(B)
    if (glm::dot(p, _direction) > sqrtf(d * d - r * r) * _cosAngle - r * _sinAngle) {
        const float AVOID_DIVIDE_BY_ZERO = 0.001f;
        return r / (d + AVOID_DIVIDE_BY_ZERO);
    }
    return PrioritizedEntity::DO_NOT_SEND;
}


void ConicalView::set(const DiffTraversal::View& view) {
    auto size = view.viewFrustums.size();
    _conicalViewFrustums.resize(size);
    for (size_t i = 0; i < size; ++i) {
        _conicalViewFrustums[i].set(view.viewFrustums[i]);
    }
}

float ConicalView::computePriority(const AACube& cube) const {
    if (_conicalViewFrustums.empty()) {
        return PrioritizedEntity::WHEN_IN_DOUBT_PRIORITY;
    }

    float priority = PrioritizedEntity::DO_NOT_SEND;

    for (const auto& view : _conicalViewFrustums) {
        priority = std::max(priority, view.computePriority(cube));
    }

    return priority;
}
