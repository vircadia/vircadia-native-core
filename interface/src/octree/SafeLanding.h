//
//  SafeLanding.h
//  interface/src/octree
//
//  Created by Simon Walton.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Controller for logic to wait for local collision bodies before enabling physics.

#ifndef hifi_SafeLanding_h
#define hifi_SafeLanding_h

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

#include "EntityItem.h"
#include "EntityDynamicInterface.h"

#include "EntityTreeRenderer.h"

class EntityTreeRenderer;
class EntityItemID;

class SafeLanding : public QObject {
public:
    void startTracking(QSharedPointer<EntityTreeRenderer> entityTreeRenderer);
    void updateTracking();
    void stopTracking();
    bool isTracking() const { return _trackingEntities; }
    bool trackingIsComplete() const;

    void finishSequence(int first, int last);  // 'last' exclusive.
    void addToSequence(int sequenceNumber);
    float loadingProgressPercentage();

private slots:
    void addTrackedEntity(const EntityItemID& entityID);
    void deleteTrackedEntity(const EntityItemID& entityID);

private:
    bool isEntityPhysicsReady(const EntityItemPointer& entity);
    void debugDumpSequenceIDs() const;

    std::mutex _lock;
    using Locker = std::lock_guard<std::mutex>;
    bool _trackingEntities { false };
    QSharedPointer<EntityTreeRenderer> _entityTreeRenderer;
    using EntityMap = std::map<EntityItemID, EntityItemPointer>;
    EntityMap _trackedEntities;

    static constexpr int INVALID_SEQUENCE = -1;
    int _initialStart { INVALID_SEQUENCE };
    int _initialEnd { INVALID_SEQUENCE };
    int _maxTrackedEntityCount { 0 };
    int _trackedEntityStabilityCount { 0 };

    quint64 _startTime { 0 };

    struct SequenceLessThan {
        bool operator()(const int& a, const int& b) const;
    };

    std::set<int, SequenceLessThan> _sequenceNumbers;

    static CalculateEntityLoadingPriority entityLoadingOperatorElevateCollidables;
    CalculateEntityLoadingPriority _prevEntityLoadingPriorityOperator { nullptr };

    static const int SEQUENCE_MODULO;
};

#endif  // hifi_SafeLanding_h
