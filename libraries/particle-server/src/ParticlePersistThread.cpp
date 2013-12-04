//
//  ParticlePersistThread.cpp
//  particle-server
//
//  Created by Brad Hefta-Gaub on 12/2/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded particle persistence
//

#include <QDebug>
#include <NodeList.h>
#include <PerfStat.h>
#include <SharedUtil.h>

#include "ParticlePersistThread.h"
#include "ParticleServer.h"

ParticlePersistThread::ParticlePersistThread(ParticleTree* tree, const char* filename, int persistInterval) :
    _tree(tree),
    _filename(filename),
    _persistInterval(persistInterval),
    _initialLoadComplete(false),
    _loadTimeUSecs(0) {
}

bool ParticlePersistThread::process() {

    if (!_initialLoadComplete) {
        uint64_t loadStarted = usecTimestampNow();
        qDebug("loading particles from file: %s...\n", _filename);

        bool persistantFileRead;

        _tree->lockForWrite();
        {
            PerformanceWarning warn(true, "Loading Particle File", true);
            persistantFileRead = _tree->readFromSVOFile(_filename);
        }
        _tree->unlock();

        _loadCompleted = time(0);
        uint64_t loadDone = usecTimestampNow();
        _loadTimeUSecs = loadDone - loadStarted;
        
        _tree->clearDirtyBit(); // the tree is clean since we just loaded it
        qDebug("DONE loading particles from file... fileRead=%s\n", debug::valueOf(persistantFileRead));
        
        unsigned long nodeCount = ParticleNode::getNodeCount();
        unsigned long internalNodeCount = ParticleNode::getInternalNodeCount();
        unsigned long leafNodeCount = ParticleNode::getLeafNodeCount();
        qDebug("Nodes after loading scene %lu nodes %lu internal %lu leaves\n", nodeCount, internalNodeCount, leafNodeCount);

        double usecPerGet = (double)ParticleNode::getGetChildAtIndexTime() / (double)ParticleNode::getGetChildAtIndexCalls();
        qDebug("getChildAtIndexCalls=%llu getChildAtIndexTime=%llu perGet=%lf \n",
            ParticleNode::getGetChildAtIndexTime(), ParticleNode::getGetChildAtIndexCalls(), usecPerGet);
            
        double usecPerSet = (double)ParticleNode::getSetChildAtIndexTime() / (double)ParticleNode::getSetChildAtIndexCalls();
        qDebug("setChildAtIndexCalls=%llu setChildAtIndexTime=%llu perSet=%lf\n",
            ParticleNode::getSetChildAtIndexTime(), ParticleNode::getSetChildAtIndexCalls(), usecPerSet);

        _initialLoadComplete = true;
        _lastCheck = usecTimestampNow(); // we just loaded, no need to save again
    }
    
    if (isStillRunning()) {
        uint64_t MSECS_TO_USECS = 1000;
        uint64_t USECS_TO_SLEEP = 100 * MSECS_TO_USECS; // every 100ms
        usleep(USECS_TO_SLEEP);
        uint64_t now = usecTimestampNow();
        uint64_t sinceLastSave = now - _lastCheck;
        uint64_t intervalToCheck = _persistInterval * MSECS_TO_USECS;
        
        if (sinceLastSave > intervalToCheck) {
            // check the dirty bit and persist here...
            _lastCheck = usecTimestampNow();
            if (_tree->isDirty()) {
                qDebug("saving particles to file %s...\n",_filename);
                _tree->writeToSVOFile(_filename);
                _tree->clearDirtyBit(); // tree is clean after saving
                qDebug("DONE saving particles to file...\n");
            }
        }
    }    
    return isStillRunning();  // keep running till they terminate us
}
