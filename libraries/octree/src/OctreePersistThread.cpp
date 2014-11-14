//
//  OctreePersistThread.cpp
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 8/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
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

bool OctreePersistThread::process() {

    if (!_initialLoadComplete) {
        quint64 loadStarted = usecTimestampNow();
        qDebug() << "loading Octrees from file: " << _filename << "...";

        bool persistantFileRead;

        _tree->lockForWrite();
        {
            PerformanceWarning warn(true, "Loading Octree File", true);
            persistantFileRead = _tree->readFromSVOFile(_filename.toLocal8Bit().constData());
            _tree->pruneTree();
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
            _lastCheck = now;
            persistOperation();
        }
    }
    return isStillRunning();  // keep running till they terminate us
}


void OctreePersistThread::aboutToFinish() {
    qDebug() << "Persist thread about to finish...";
    persistOperation();
    qDebug() << "Persist thread done with about to finish...";
}

void OctreePersistThread::persistOperation() {
    if (_tree->isDirty()) {
        _tree->lockForWrite();
        {
            qDebug() << "pruning Octree before saving...";
            _tree->pruneTree();
            qDebug() << "DONE pruning Octree before saving...";
        }
        _tree->unlock();

        qDebug() << "saving Octree to file " << _filename << "...";
        _tree->writeToSVOFile(_filename.toLocal8Bit().constData());
        _tree->clearDirtyBit(); // tree is clean after saving
        qDebug("DONE saving Octree to file...");
    }
}
