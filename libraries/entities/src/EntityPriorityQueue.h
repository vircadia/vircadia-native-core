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
#include <unordered_set>

#include "EntityItem.h"

// PrioritizedEntity is a placeholder in a sorted queue.
class PrioritizedEntity {
public:
    static constexpr float DO_NOT_SEND { -1.0e6f };
    static constexpr float FORCE_REMOVE { -1.0e5f };
    static constexpr float WHEN_IN_DOUBT_PRIORITY { 1.0f };

    PrioritizedEntity(EntityItemPointer entity, float priority, bool forceRemove = false) : _weakEntity(entity), _rawEntityPointer(entity.get()), _priority(priority), _forceRemove(forceRemove) {}
    EntityItemPointer getEntity() const { return _weakEntity.lock(); }
    EntityItem* getRawEntityPointer() const { return _rawEntityPointer; }
    float getPriority() const { return _priority; }
    bool shouldForceRemove() const { return _forceRemove; }

    class Compare {
    public:
        bool operator() (const PrioritizedEntity& A, const PrioritizedEntity& B) { return A._priority < B._priority; }
    };
    friend class Compare;

private:
    EntityItemWeakPointer _weakEntity;
    EntityItem* _rawEntityPointer;
    float _priority;
    bool _forceRemove;
};

class EntityPriorityQueue {
public:
    inline bool empty() const {
        assert(_queue.empty() == _entities.empty());
        return _queue.empty();
    }

    inline const PrioritizedEntity& top() const {
        assert(!_queue.empty());
        return _queue.top();
    }

    inline bool contains(const EntityItem* entity) const {
        return _entities.find(entity) != std::end(_entities);
    }

    inline void emplace(const EntityItemPointer& entity, float priority, bool forceRemove = false) {
        assert(entity && !contains(entity.get()));
        _queue.emplace(entity, priority, forceRemove);
        _entities.insert(entity.get());
        assert(_queue.size() == _entities.size());
    }

    inline void pop() {
        assert(!empty());
        _entities.erase(_queue.top().getRawEntityPointer());
        _queue.pop();
        assert(_queue.size() == _entities.size());
    }

    inline void swap(EntityPriorityQueue& other) {
        std::swap(_queue, other._queue);
        std::swap(_entities, other._entities);
    }

private:
    using PriorityQueue = std::priority_queue<PrioritizedEntity,
                                              std::vector<PrioritizedEntity>,
                                              PrioritizedEntity::Compare>;

    PriorityQueue _queue;
    // Keep dictionary of all the entities in the queue for fast contain checks.
    std::unordered_set<const EntityItem*> _entities;

};

#endif // hifi_EntityPriorityQueue_h
