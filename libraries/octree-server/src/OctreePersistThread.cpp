//
//  OctreePersistThread.cpp
//  Octree-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded Octree persistence
//

#include <QDebug>
#include <PerfStat.h>
#include <SharedUtil.h>

#include "OctreePersistThread.h"
#include "OctreeServer.h"

OctreePersistThread::OctreePersistThread(Octree* tree, const char* filename, int persistInterval) :
    _tree(tree),
    _filename(filename),
    _persistInterval(persistInterval),
    _initialLoadComplete(false),
    _loadTimeUSecs(0) {
}

bool OctreePersistThread::process() {

    if (!_initialLoadComplete) {
        uint64_t loadStarted = usecTimestampNow();
        qDebug("loading Octrees from file: %s...\n", _filename);

        bool persistantFileRead;

        _tree->lockForWrite();
        {
            PerformanceWarning warn(true, "Loading Octree File", true);
            persistantFileRead = _tree->readFromSVOFile(_filename);
        }
        _tree->unlock();

        _loadCompleted = time(0);
        uint64_t loadDone = usecTimestampNow();
        _loadTimeUSecs = loadDone - loadStarted;
        
        _tree->clearDirtyBit(); // the tree is clean since we just loaded it
        qDebug("DONE loading Octrees from file... fileRead=%s\n", debug::valueOf(persistantFileRead));
        
        unsigned long nodeCount = OctreeElement::getNodeCount();
        unsigned long internalNodeCount = OctreeElement::getInternalNodeCount();
        unsigned long leafNodeCount = OctreeElement::getLeafNodeCount();
        qDebug("Nodes after loading scene %lu nodes %lu internal %lu leaves\n", nodeCount, internalNodeCount, leafNodeCount);

        double usecPerGet = (double)OctreeElement::getGetChildAtIndexTime() / (double)OctreeElement::getGetChildAtIndexCalls();
        qDebug("getChildAtIndexCalls=%llu getChildAtIndexTime=%llu perGet=%lf \n",
            OctreeElement::getGetChildAtIndexTime(), OctreeElement::getGetChildAtIndexCalls(), usecPerGet);
            
        double usecPerSet = (double)OctreeElement::getSetChildAtIndexTime() / (double)OctreeElement::getSetChildAtIndexCalls();
        qDebug("setChildAtIndexCalls=%llu setChildAtIndexTime=%llu perSet=%lf\n",
            OctreeElement::getSetChildAtIndexTime(), OctreeElement::getSetChildAtIndexCalls(), usecPerSet);

        _initialLoadComplete = true;
        _lastCheck = usecTimestampNow(); // we just loaded, no need to save again
    }
    
    if (isStillRunning()) {
        uint64_t MSECS_TO_USECS = 1000;
        uint64_t USECS_TO_SLEEP = 10 * MSECS_TO_USECS; // every 10ms
        usleep(USECS_TO_SLEEP);
        
        // do our updates then check to save...
        _tree->lockForWrite();
        _tree->update();
        _tree->unlock();
        
        uint64_t now = usecTimestampNow();
        uint64_t sinceLastSave = now - _lastCheck;
        uint64_t intervalToCheck = _persistInterval * MSECS_TO_USECS;
        
        if (sinceLastSave > intervalToCheck) {
            // check the dirty bit and persist here...
            _lastCheck = usecTimestampNow();
            if (_tree->isDirty()) {
                qDebug("saving Octrees to file %s...\n",_filename);
                _tree->writeToSVOFile(_filename);
                _tree->clearDirtyBit(); // tree is clean after saving
                qDebug("DONE saving Octrees to file...\n");
            }
        }
    }    
    return isStillRunning();  // keep running till they terminate us
}
