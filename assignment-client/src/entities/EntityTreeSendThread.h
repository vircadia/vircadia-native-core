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

#include "EntityPriorityQueue.h"

class EntityNodeData;
class EntityItem;

class EntityTreeSendThread : public OctreeSendThread {
    Q_OBJECT

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

    void startNewTraversal(const ViewFrustum& viewFrustum, EntityTreeElementPointer root, int32_t lodLevelOffset);
    bool traverseTreeAndBuildNextPacketPayload(EncodeBitstreamParams& params) override;

    DiffTraversal _traversal;
    EntityPriorityQueue _sendQueue;
    std::unordered_set<EntityItem*> _entitiesInQueue;
    std::unordered_map<EntityItem*, uint64_t> _knownState;
    ConicalView _conicalView; // cached optimized view for fast priority calculations

    // packet construction stuff
    EntityTreeElementExtraEncodeDataPointer _extraEncodeData { new EntityTreeElementExtraEncodeData() };
    int32_t _numEntitiesOffset { 0 };
    uint16_t _numEntities { 0 };

private slots:
    void deletingEntityPointer(EntityItem* entity);
};

#endif // hifi_EntityTreeSendThread_h
