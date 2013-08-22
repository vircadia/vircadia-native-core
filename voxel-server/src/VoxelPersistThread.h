//
//  VoxelPersistThread.h
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded voxel persistance
//

#ifndef __voxel_server__VoxelPersistThread__
#define __voxel_server__VoxelPersistThread__

#include <GenericThread.h>
#include <NetworkPacket.h>
#include <VoxelTree.h>

/// Generalized threaded processor for handling received inbound packets. 
class VoxelPersistThread : public virtual GenericThread {
public:
    static const int DEFAULT_PERSIST_INTERVAL = 1000 * 30; // every 30 seconds

    VoxelPersistThread(VoxelTree* tree, const char* filename, int persistInterval = DEFAULT_PERSIST_INTERVAL);
protected:
    /// Implements generic processing behavior for this thread.
    virtual bool process();
private:
    VoxelTree* _tree;
    const char* _filename;
    int _persistInterval;
};

#endif // __voxel_server__VoxelPersistThread__
