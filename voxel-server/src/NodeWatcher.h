//
//  NodeWatcher.h
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Node List Hook that watches for Node's being killed in order to clean up node specific memory and threads
//

#ifndef __voxel_server__NodeWatcher__
#define __voxel_server__NodeWatcher__

#include <NodeList.h>

/// Voxel server's node watcher, which watches for nodes being killed and cleans up their data and threads
class NodeWatcher : public virtual NodeListHook {
public:
    virtual void nodeAdded(Node* node);
    virtual void nodeKilled(Node* node);
};

#endif // __voxel_server__NodeWatcher__
