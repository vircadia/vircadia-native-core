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
#include <EntityTreeElement.h>

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
    static const float DO_NOT_SEND;

    PrioritizedEntity(EntityItemPointer entity, float priority) : _weakEntity(entity), _rawEntityPointer(entity.get()), _priority(priority) {}
    float updatePriority(const ConicalView& view);
    EntityItemPointer getEntity() const { return _weakEntity.lock(); }
    EntityItem* getRawEntityPointer() const { return _rawEntityPointer; }
    float getPriority() const { return _priority; }

    class Compare {
    public:
        bool operator() (const PrioritizedEntity& A, const PrioritizedEntity& B) { return A._priority < B._priority; }
    };
    friend class Compare;

private:
    EntityItemWeakPointer _weakEntity;
    EntityItem* _rawEntityPointer;
    float _priority;
};

using EntityPriorityQueue = std::priority_queue< PrioritizedEntity, std::vector<PrioritizedEntity>, PrioritizedEntity::Compare >;

#endif // hifi_EntityPriorityQueue_h
