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

const float DO_NOT_SEND = -1.0e-6f;

void ConicalView::set(const ViewFrustum& viewFrustum) {
    // The ConicalView has two parts: a central sphere (same as ViewFrustm) and a circular cone that bounds the frustum part.
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

float ConicalView::computePriority(const AACube& cube) const {
    glm::vec3 p = cube.calcCenter() - _position; // position of bounding sphere in view-frame
    float d = glm::length(p); // distance to center of bounding sphere
    float r = 0.5f * cube.getScale(); // radius of bounding sphere
    if (d < _radius + r) {
        return r;
    }
    if (glm::dot(p, _direction) > sqrtf(d * d - r * r) * _cosAngle - r * _sinAngle) {
        const float AVOID_DIVIDE_BY_ZERO = 0.001f;
        return r / (d + AVOID_DIVIDE_BY_ZERO);
    }
    return DO_NOT_SEND;
}

// static
float ConicalView::computePriority(const EntityItemPointer& entity) const {
    assert(entity);
    bool success;
    AACube cube = entity->getQueryAACube(success);
    if (success) {
        return computePriority(cube);
    } else {
        // when in doubt give it something positive
        return 1.0f;
    }
}

float PrioritizedEntity::updatePriority(const ConicalView& conicalView) {
    EntityItemPointer entity = _weakEntity.lock();
    if (entity) {
        _priority = conicalView.computePriority(entity);
    } else {
        _priority = DO_NOT_SEND;
    }
    return _priority;
}

TraversalWaypoint::TraversalWaypoint(EntityTreeElementPointer& element) : _nextIndex(0) {
    assert(element);
    _weakElement = element;
}

void TraversalWaypoint::getNextVisibleElementFirstTime(VisibleElement& next, const ViewFrustum& view) {
    // NOTE: no need to set next.intersection in the "FirstTime" context
    if (_nextIndex == -1) {
        // only get here for the root TraversalWaypoint at the very beginning of traversal
        // safe to assume this element intersects view
        ++_nextIndex;
        next.element = _weakElement.lock();
        return;
    } else if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement && view.cubeIntersectsKeyhole(nextElement->getAACube())) {
                    next.element = nextElement;
                    return;
                }
            }
        }
    }
    next.element.reset();
}

void TraversalWaypoint::getNextVisibleElementAgain(VisibleElement& next, const ViewFrustum& view, uint64_t lastTime) {
    if (_nextIndex == -1) {
        // only get here for the root TraversalWaypoint at the very beginning of traversal
        // safe to assume this element intersects view
        ++_nextIndex;
        EntityTreeElementPointer element = _weakElement.lock();
        // root case is special: its intersection is always INTERSECT
        // and we can skip it if the content hasn't changed
        if (element->getLastChangedContent() > lastTime) {
            next.element = element;
            next.intersection = ViewFrustum::INTERSECT;
            return;
        }
    }
    if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement && nextElement->getLastChanged() > lastTime) {
                    ViewFrustum::intersection intersection = view.calculateCubeKeyholeIntersection(nextElement->getAACube());
                    if (intersection != ViewFrustum::OUTSIDE) {
                        next.element = nextElement;
                        next.intersection = intersection;
                        return;
                    }
                }
            }
        }
    }
    next.element.reset();
    next.intersection = ViewFrustum::OUTSIDE;
}

void TraversalWaypoint::getNextVisibleElementDifferential(VisibleElement& next,
        const ViewFrustum& view, const ViewFrustum& lastView, uint64_t lastTime) {
    if (_nextIndex == -1) {
        // only get here for the root TraversalWaypoint at the very beginning of traversal
        // safe to assume this element intersects view
        ++_nextIndex;
        EntityTreeElementPointer element = _weakElement.lock();
        // root case is special: its intersection is always INTERSECT
        // and we can skip it if the content hasn't changed
        if (element->getLastChangedContent() > lastTime) {
            next.element = element;
            next.intersection = ViewFrustum::INTERSECT;
            return;
        }
    }
    if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement) {
                    AACube cube = nextElement->getAACube();
                    // NOTE: for differential case next.intersection is against the _completedView
                    ViewFrustum::intersection intersection = lastView.calculateCubeKeyholeIntersection(cube);
                    if ( lastView.calculateCubeKeyholeIntersection(cube) != ViewFrustum::OUTSIDE &&
                            !(intersection == ViewFrustum::INSIDE && nextElement->getLastChanged() < lastTime)) {
                        next.element = nextElement;
                        next.intersection = intersection;
                        return;
                    }
                }
            }
        }
    }
    next.element.reset();
    next.intersection = ViewFrustum::OUTSIDE;
}
