//
//  VoxelPersistThread.cpp
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded voxel persistence
//

#include <QDebug>
#include <NodeList.h>
#include <PerfStat.h>
#include <SharedUtil.h>

#include "VoxelPersistThread.h"
#include "VoxelServer.h"

VoxelPersistThread::VoxelPersistThread(VoxelTree* tree, const char* filename, int persistInterval) :
    _tree(tree),
    _filename(filename),
    _persistInterval(persistInterval),
    _initialLoadComplete(false),
    _loadTimeUSecs(0) {
}

bool VoxelPersistThread::process() {

    if (!_initialLoadComplete) {
        uint64_t loadStarted = usecTimestampNow();
        qDebug("loading voxels from file: %s...\n", _filename);

        bool persistantFileRead;

        _tree->lockForWrite();
        {
            PerformanceWarning warn(true, "Loading Voxel File", true);
            persistantFileRead = _tree->readFromSVOFile(_filename);
        }
        _tree->unlock();

        _loadCompleted = time(0);
        uint64_t loadDone = usecTimestampNow();
        _loadTimeUSecs = loadDone - loadStarted;
        
        _tree->clearDirtyBit(); // the tree is clean since we just loaded it
        qDebug("DONE loading voxels from file... fileRead=%s\n", debug::valueOf(persistantFileRead));
        
        unsigned long nodeCount = VoxelNode::getNodeCount();
        unsigned long internalNodeCount = VoxelNode::getInternalNodeCount();
        unsigned long leafNodeCount = VoxelNode::getLeafNodeCount();
        qDebug("Nodes after loading scene %lu nodes %lu internal %lu leaves\n", nodeCount, internalNodeCount, leafNodeCount);

        double usecPerGet = (double)VoxelNode::getGetChildAtIndexTime() / (double)VoxelNode::getGetChildAtIndexCalls();
        qDebug("getChildAtIndexCalls=%llu getChildAtIndexTime=%llu perGet=%lf \n",
            VoxelNode::getGetChildAtIndexTime(), VoxelNode::getGetChildAtIndexCalls(), usecPerGet);
            
        double usecPerSet = (double)VoxelNode::getSetChildAtIndexTime() / (double)VoxelNode::getSetChildAtIndexCalls();
        qDebug("setChildAtIndexCalls=%llu setChildAtIndexTime=%llu perSet=%lf\n",
            VoxelNode::getSetChildAtIndexTime(), VoxelNode::getSetChildAtIndexCalls(), usecPerSet);

        _initialLoadComplete = true;
        _lastSave = usecTimestampNow(); // we just loaded, no need to save again
    }
    
    if (isStillRunning()) {
        uint64_t MSECS_TO_USECS = 1000;
        uint64_t USECS_TO_SLEEP = 100 * MSECS_TO_USECS; // every 100ms
        usleep(USECS_TO_SLEEP);
    
        if ((usecTimestampNow() - _lastSave) > (_persistInterval * MSECS_TO_USECS)) {
            qDebug("checking if voxels are dirty.\n");
            // check the dirty bit and persist here...
            if (_tree->isDirty()) {
                _lastSave = usecTimestampNow();
                qDebug("saving voxels to file %s...\n",_filename);
                _tree->writeToSVOFile(_filename);
                _tree->clearDirtyBit(); // tree is clean after saving
                qDebug("DONE saving voxels to file...\n");
            }
        }
    }    
    return isStillRunning();  // keep running till they terminate us
}
