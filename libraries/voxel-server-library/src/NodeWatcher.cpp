//
//  NodeWatcher.h
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Node List Hook that watches for Node's being killed in order to clean up node specific memory and threads
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
        if (nodeData) {
            node->setLinkedData(NULL);
            delete nodeData;
        }
    }
};
