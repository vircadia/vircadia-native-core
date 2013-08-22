//
//  VoxelPersistThread.cpp
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded voxel persistance
//

#include <NodeList.h>
#include <SharedUtil.h>

#include "VoxelPersistThread.h"
#include "VoxelServer.h"

VoxelPersistThread::VoxelPersistThread(VoxelTree* tree, const char* filename, int persistInterval) :
    _tree(tree),
    _filename(filename),
    _persistInterval(persistInterval) {
}

bool VoxelPersistThread::process() {
    uint64_t MSECS_TO_USECS = 1000;
    usleep(_persistInterval * MSECS_TO_USECS);


    // check the dirty bit and persist here...
    if (_tree->isDirty()) {
        printf("saving voxels to file %s...\n",_filename);
        _tree->writeToSVOFile(_filename);
        _tree->clearDirtyBit(); // tree is clean after saving
        printf("DONE saving voxels to file...\n");
    }

    return isStillRunning();  // keep running till they terminate us
}
