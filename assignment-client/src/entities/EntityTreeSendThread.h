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

#include <AACube.h>

#include "EntityTreeElement.h"

const float SQRT_TWO_OVER_TWO = 0.7071067811865;
const float DEFAULT_VIEW_RADIUS = 10.0f;

class EntityNodeData;
class EntityItem;


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

class VisibleElement {
public:
    EntityTreeElementPointer element;
    ViewFrustum::intersection intersection { ViewFrustum::OUTSIDE };
};

class Fork {
public:
    Fork(EntityTreeElementPointer& element);

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


class EntityTreeSendThread : public OctreeSendThread {
public:
    EntityTreeSendThread(OctreeServer* myServer, const SharedNodePointer& node);

protected:
    void preDistributionProcessing() override;
    void traverseTreeAndSendContents(SharedNodePointer node, OctreeQueryNode* nodeData,
            bool viewFrustumChanged, bool isFullScene) override;

private:
    // the following two methods return booleans to indicate if any extra flagged entities were new additions to set
    bool addAncestorsToExtraFlaggedEntities(const QUuid& filteredEntityID, EntityItem& entityItem, EntityNodeData& nodeData);
    bool addDescendantsToExtraFlaggedEntities(const QUuid& filteredEntityID, EntityItem& entityItem, EntityNodeData& nodeData);

    void startNewTraversal(const ViewFrustum& viewFrustum, EntityTreeElementPointer root);
    void getNextVisibleElement(VisibleElement& element);
    void dump() const; // DEBUG method, delete later

    EntityPriorityQueue _sendQueue;
    ViewFrustum _currentView;
    ViewFrustum _completedView;
    ConicalView _conicalView; // optimized view for fast priority calculations
    std::vector<Fork> _traversalPath;
    std::function<void (VisibleElement&)> _getNextVisibleElementCallback { nullptr };
    std::function<void (VisibleElement&)> _scanNextElementCallback { nullptr };
    uint64_t _startOfCompletedTraversal { 0 };
    uint64_t _startOfCurrentTraversal { 0 };
};

#endif // hifi_EntityTreeSendThread_h
