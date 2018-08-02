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

#include <unordered_set>

#include "../octree/OctreeSendThread.h"

#include <DiffTraversal.h>
#include <EntityPriorityQueue.h>
#include <shared/ConicalViewFrustum.h>


class EntityNodeData;
class EntityItem;

class EntityTreeSendThread : public OctreeSendThread {
    Q_OBJECT

public:
    EntityTreeSendThread(OctreeServer* myServer, const SharedNodePointer& node);

protected:
    bool traverseTreeAndSendContents(SharedNodePointer node, OctreeQueryNode* nodeData,
            bool viewFrustumChanged, bool isFullScene) override;

private slots:
    void resetState(); // clears our known state forcing entities to appear unsent

private:
    // the following two methods return booleans to indicate if any extra flagged entities were new additions to set
    bool addAncestorsToExtraFlaggedEntities(const QUuid& filteredEntityID, EntityItem& entityItem, EntityNodeData& nodeData);
    bool addDescendantsToExtraFlaggedEntities(const QUuid& filteredEntityID, EntityItem& entityItem, EntityNodeData& nodeData);

    void startNewTraversal(const DiffTraversal::View& viewFrustum, EntityTreeElementPointer root);
    bool traverseTreeAndBuildNextPacketPayload(EncodeBitstreamParams& params, const QJsonObject& jsonFilters) override;

    void preDistributionProcessing() override;
    bool hasSomethingToSend(OctreeQueryNode* nodeData) override { return !_sendQueue.empty(); }
    bool shouldStartNewTraversal(OctreeQueryNode* nodeData, bool viewFrustumChanged) override { return viewFrustumChanged || _traversal.finished(); }

    DiffTraversal _traversal;
    EntityPriorityQueue _sendQueue;
    std::unordered_map<EntityItem*, uint64_t> _knownState;

    // packet construction stuff
    EntityTreeElementExtraEncodeDataPointer _extraEncodeData { new EntityTreeElementExtraEncodeData() };
    int32_t _numEntitiesOffset { 0 };
    uint16_t _numEntities { 0 };

private slots:
    void editingEntityPointer(const EntityItemPointer& entity);
    void deletingEntityPointer(EntityItem* entity);
};

#endif // hifi_EntityTreeSendThread_h
