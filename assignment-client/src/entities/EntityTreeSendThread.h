//
//  EntityTreeSendThread.h
//  assignment-client/src/entities
//
//  Created by Stephen Birarda on 2/15/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityTreeSendThread_h
#define hifi_EntityTreeSendThread_h

#include <queue>

#include "../octree/OctreeSendThread.h"

#include "EntityTreeElement.h"

class EntityNodeData;
class EntityItem;

class PrioritizedEntity {
public:
    PrioritizedEntity(EntityItemPointer entity) : _weakEntity(entity) { }
    void updatePriority(const ViewFrustum& view);
    EntityItemPointer getEntity() const { return _weakEntity.lock(); }
    float getPriority() const { return _priority; }

    class Compare {
    public:
        bool operator() (const PrioritizedEntity& A, const PrioritizedEntity& B) { return A._priority < B._priority; }
    };

    friend class Compare;

private:
    EntityItemWeakPointer _weakEntity;
    float _priority { 0.0f };
};
using EntityPriorityQueue = std::priority_queue< PrioritizedEntity, std::vector<PrioritizedEntity>, PrioritizedEntity::Compare >;

class TreeTraversalPath {
public:
    TreeTraversalPath();

    void startNewTraversal(const ViewFrustum& view, EntityTreeElementPointer root);

    EntityTreeElementPointer getNextElement();

    const ViewFrustum& getView() const { return _currentView; }

    bool empty() const { return _forks.empty(); }
    size_t size() const { return _forks.size(); } // adebug
    void dump() const;

    class Fork {
    public:
        Fork(EntityTreeElementPointer& element);

        EntityTreeElementPointer getNextElement(const ViewFrustum& view);
        EntityTreeElementPointer getNextElementAgain(const ViewFrustum& view, uint64_t oldTime);
        EntityTreeElementPointer getNextElementDelta(const ViewFrustum& newView, const ViewFrustum& oldView, uint64_t oldTime);
        int8_t getNextIndex() const { return _nextIndex; }
        void initRootNextIndex() { _nextIndex = -1; }

    protected:
        EntityTreeElementWeakPointer _weakElement;
        int8_t _nextIndex;
    };

protected:
    EntityTreeElementPointer traverseFirstTime();
    EntityTreeElementPointer traverseAgain();
    EntityTreeElementPointer traverseDelta();
    void onCompleteTraversal();

    ViewFrustum _currentView;
    ViewFrustum _lastCompletedView;
    std::vector<Fork> _forks;
    std::function<EntityTreeElementPointer()> _traversalCallback { nullptr };
    uint64_t _startOfLastCompletedTraversal { 0 };
    uint64_t _startOfCurrentTraversal { 0 };
};


class EntityTreeSendThread : public OctreeSendThread {

public:
    EntityTreeSendThread(OctreeServer* myServer, const SharedNodePointer& node) : OctreeSendThread(myServer, node) {};

protected:
    void preDistributionProcessing() override;
    void traverseTreeAndSendContents(SharedNodePointer node, OctreeQueryNode* nodeData,
            bool viewFrustumChanged, bool isFullScene) override;

private:
    // the following two methods return booleans to indicate if any extra flagged entities were new additions to set
    bool addAncestorsToExtraFlaggedEntities(const QUuid& filteredEntityID, EntityItem& entityItem, EntityNodeData& nodeData);
    bool addDescendantsToExtraFlaggedEntities(const QUuid& filteredEntityID, EntityItem& entityItem, EntityNodeData& nodeData);

    TreeTraversalPath _path;
    EntityPriorityQueue _sendQueue;
};

#endif // hifi_EntityTreeSendThread_h
