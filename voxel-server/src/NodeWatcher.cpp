//
//  NodeWatcher.h
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded object for sending voxels to a client
//

#include <NodeList.h>
#include "NodeWatcher.h"
#include "VoxelNodeData.h"

void NodeWatcher::nodeAdded(Node* node) {
    // do nothing
}

void NodeWatcher::nodeKilled(Node* node) {
    // Use this to cleanup our node
    if (node->getType() == NODE_TYPE_AGENT) {
        VoxelNodeData* nodeData = (VoxelNodeData*)node->getLinkedData();
        node->setLinkedData(NULL);
        delete nodeData;
    }
};
