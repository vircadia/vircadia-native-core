//
//  EntityPriorityQueue.h
//  assignment-client/src/entities
//
//  Created by Andrew Meadows 2017.08.08
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityPriorityQueue_h
#define hifi_EntityPriorityQueue_h

#include <queue>

#include <AACube.h>

#include "EntityTreeElement.h"

const float SQRT_TWO_OVER_TWO = 0.7071067811865f;
const float DEFAULT_VIEW_RADIUS = 10.0f;

// ConicalView is an approximation of a ViewFrustum for fast calculation of sort priority.
class ConicalView {
public:
    ConicalView() {}
    ConicalView(const ViewFrustum& viewFrustum) { set(viewFrustum); }
    void set(const ViewFrustum& viewFrustum);
    float computePriority(const AACube& cube) const;
    float computePriority(const EntityItemPointer& entity) const;
private:
    glm::vec3 _position { 0.0f, 0.0f, 0.0f };
    glm::vec3 _direction { 0.0f, 0.0f, 1.0f };
    float _sinAngle { SQRT_TWO_OVER_TWO };
    float _cosAngle { SQRT_TWO_OVER_TWO };
    float _radius { DEFAULT_VIEW_RADIUS };
};

// PrioritizedEntity is a placeholder in a sorted queue.
class PrioritizedEntity {
public:
    PrioritizedEntity(EntityItemPointer entity, float priority) : _weakEntity(entity), _priority(priority) { }
    float updatePriority(const ConicalView& view);
    EntityItemPointer getEntity() const { return _weakEntity.lock(); }
    float getPriority() const { return _priority; }

    class Compare {
    public:
        bool operator() (const PrioritizedEntity& A, const PrioritizedEntity& B) { return A._priority < B._priority; }
    };
    friend class Compare;

private:
    EntityItemWeakPointer _weakEntity;
    float _priority;
};

// VisibleElement is a struct identifying an element and how it intersected the view.
// The intersection is used to optimize culling entities from the sendQueue.
class VisibleElement {
public:
    EntityTreeElementPointer element;
    ViewFrustum::intersection intersection { ViewFrustum::OUTSIDE };
};

// TraversalWaypoint is an bookmark in a "path" of waypoints during a traversal.
class TraversalWaypoint {
public:
    TraversalWaypoint(EntityTreeElementPointer& element);

    void getNextVisibleElementFirstTime(VisibleElement& next, const ViewFrustum& view);
    void getNextVisibleElementAgain(VisibleElement& next, const ViewFrustum& view, uint64_t lastTime);
    void getNextVisibleElementDifferential(VisibleElement& next, const ViewFrustum& view, const ViewFrustum& lastView, uint64_t lastTime);

    int8_t getNextIndex() const { return _nextIndex; }
    void initRootNextIndex() { _nextIndex = -1; }

protected:
    EntityTreeElementWeakPointer _weakElement;
    int8_t _nextIndex;
};

using EntityPriorityQueue = std::priority_queue< PrioritizedEntity, std::vector<PrioritizedEntity>, PrioritizedEntity::Compare >;

#endif // hifi_EntityPriorityQueue_h
