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

#include "../octree/OctreeSendThread.h"

class EntityNodeData;
class EntityItem;

class EntityTreeSendThread : public OctreeSendThread {

public:
    EntityTreeSendThread(OctreeServer* myServer, const SharedNodePointer& node) : OctreeSendThread(myServer, node) {};

protected:
    virtual void preDistributionProcessing() override;

private:
    // the following two methods return booleans to indicate if any extra flagged entities were new additions to set
    bool addAncestorsToExtraFlaggedEntities(const QUuid& filteredEntityID, EntityItem& entityItem, EntityNodeData& nodeData);
    bool addDescendantsToExtraFlaggedEntities(const QUuid& filteredEntityID, EntityItem& entityItem, EntityNodeData& nodeData);

};

#endif // hifi_EntityTreeSendThread_h
