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

OctreePersistThread::OctreePersistThread(Octree* tree, const QString& filename, int persistInterval) :
    _tree(tree),
    _filename(filename),
    _persistInterval(persistInterval),
    _initialLoadComplete(false),
    _loadTimeUSecs(0) 
{
}

OctreePersistThread::~OctreePersistThread() {
    qDebug() << "OctreePersistThread::~OctreePersistThread()...";
}

bool OctreePersistThread::process() {

    if (!_initialLoadComplete) {
        quint64 loadStarted = usecTimestampNow();
        qDebug() << "loading Octrees from file: " << _filename << "...";

        bool persistantFileRead;

        _tree->lockForWrite();
        {
            PerformanceWarning warn(true, "Loading Octree File", true);
            persistantFileRead = _tree->readFromSVOFile(_filename.toLocal8Bit().constData());
        }
        _tree->unlock();

        quint64 loadDone = usecTimestampNow();
        _loadTimeUSecs = loadDone - loadStarted;

        _tree->clearDirtyBit(); // the tree is clean since we just loaded it
        qDebug("DONE loading Octrees from file... fileRead=%s", debug::valueOf(persistantFileRead));

        unsigned long nodeCount = OctreeElement::getNodeCount();
        unsigned long internalNodeCount = OctreeElement::getInternalNodeCount();
        unsigned long leafNodeCount = OctreeElement::getLeafNodeCount();
        qDebug("Nodes after loading scene %lu nodes %lu internal %lu leaves", nodeCount, internalNodeCount, leafNodeCount);

        double usecPerGet = (double)OctreeElement::getGetChildAtIndexTime() / (double)OctreeElement::getGetChildAtIndexCalls();
        qDebug() << "getChildAtIndexCalls=" << OctreeElement::getGetChildAtIndexCalls()
                << " getChildAtIndexTime=" << OctreeElement::getGetChildAtIndexTime() << " perGet=" << usecPerGet;

        double usecPerSet = (double)OctreeElement::getSetChildAtIndexTime() / (double)OctreeElement::getSetChildAtIndexCalls();
        qDebug() << "setChildAtIndexCalls=" << OctreeElement::getSetChildAtIndexCalls()
                << " setChildAtIndexTime=" << OctreeElement::getSetChildAtIndexTime() << " perset=" << usecPerSet;

        _initialLoadComplete = true;
        _lastCheck = usecTimestampNow(); // we just loaded, no need to save again

        emit loadCompleted();
    }

    if (isStillRunning()) {
        quint64 MSECS_TO_USECS = 1000;
        quint64 USECS_TO_SLEEP = 10 * MSECS_TO_USECS; // every 10ms
        usleep(USECS_TO_SLEEP);

        // do our updates then check to save...
        _tree->update();

        quint64 now = usecTimestampNow();
        quint64 sinceLastSave = now - _lastCheck;
        quint64 intervalToCheck = _persistInterval * MSECS_TO_USECS;

        if (sinceLastSave > intervalToCheck) {
            // check the dirty bit and persist here...
            _lastCheck = usecTimestampNow();
            if (_tree->isDirty()) {
                qDebug() << "saving Octrees to file " << _filename << "...";
                _tree->writeToSVOFile(_filename.toLocal8Bit().constData());
                _tree->clearDirtyBit(); // tree is clean after saving
                qDebug("DONE saving Octrees to file...");
            }
        }
    }
    return isStillRunning();  // keep running till they terminate us
}
