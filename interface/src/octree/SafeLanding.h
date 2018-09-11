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

class EntityTreeRenderer;
class EntityItemID;

class SafeLanding : public QObject {
public:
    void startEntitySequence(QSharedPointer<EntityTreeRenderer> entityTreeRenderer);
    void stopEntitySequence();
    void setCompletionSequenceNumbers(int first, int last);  // 'last' exclusive.
    void noteReceivedsequenceNumber(int sequenceNumber);
    bool isLoadSequenceComplete();
    float loadingProgressPercentage();

private slots:
    void addTrackedEntity(const EntityItemID& entityID);
    void deleteTrackedEntity(const EntityItemID& entityID);

private:
    bool isSequenceNumbersComplete();
    void debugDumpSequenceIDs() const;
    bool isEntityLoadingComplete();

    std::mutex _lock;
    using Locker = std::lock_guard<std::mutex>;
    bool _trackingEntities { false };
    EntityTreePointer _entityTree;
    using EntityMap = std::map<EntityItemID, EntityItemPointer>;
    EntityMap _trackedEntities;

    static constexpr int INVALID_SEQUENCE = -1;
    int _initialStart { INVALID_SEQUENCE };
    int _initialEnd { INVALID_SEQUENCE };
    int _maxTrackedEntityCount { 0 };

    struct SequenceLessThan {
        bool operator()(const int& a, const int& b) const;
    };

    std::set<int, SequenceLessThan> _sequenceNumbers;

    static float ElevatedPriority(const EntityItem& entityItem);
    static float StandardPriority(const EntityItem&) { return 0.0f; }

    static const int SEQUENCE_MODULO;
};

#endif  // hifi_SafeLanding_h
