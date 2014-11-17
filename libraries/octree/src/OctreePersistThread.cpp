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

#include <time.h>

#include <QDebug>
#include <PerfStat.h>
#include <SharedUtil.h>

#include "OctreePersistThread.h"

const int OctreePersistThread::DEFAULT_PERSIST_INTERVAL = 1000 * 30; // every 30 seconds
const int OctreePersistThread::DEFAULT_BACKUP_INTERVAL = 1000 * 60 * 30; // every 30 minutes
const QString OctreePersistThread::DEFAULT_BACKUP_EXTENSION_FORMAT(".backup.%Y-%m-%d.%H:%M:%S.%z");


OctreePersistThread::OctreePersistThread(Octree* tree, const QString& filename, int persistInterval, bool wantBackup, 
                                            int backupInterval, const QString& backupExtensionFormat, bool debugTimestampNow) :
    _tree(tree),
    _filename(filename),
    _backupExtensionFormat(backupExtensionFormat),
    _persistInterval(persistInterval),
    _backupInterval(backupInterval),
    _initialLoadComplete(false),
    _loadTimeUSecs(0),
    _lastCheck(0),
    _lastBackup(0),
    _wantBackup(wantBackup),
    _debugTimestampNow(debugTimestampNow),
    _lastTimeDebug(0)
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

        bool wantDebug = false;
        if (wantDebug) {
            double usecPerGet = (double)OctreeElement::getGetChildAtIndexTime() 
                                    / (double)OctreeElement::getGetChildAtIndexCalls();
            qDebug() << "getChildAtIndexCalls=" << OctreeElement::getGetChildAtIndexCalls()
                    << " getChildAtIndexTime=" << OctreeElement::getGetChildAtIndexTime() << " perGet=" << usecPerGet;

            double usecPerSet = (double)OctreeElement::getSetChildAtIndexTime() 
                                    / (double)OctreeElement::getSetChildAtIndexCalls();
            qDebug() << "setChildAtIndexCalls=" << OctreeElement::getSetChildAtIndexCalls()
                    << " setChildAtIndexTime=" << OctreeElement::getSetChildAtIndexTime() << " perSet=" << usecPerSet;
        }

        _initialLoadComplete = true;
        _lastBackup = _lastCheck = usecTimestampNow(); // we just loaded, no need to save again
        time(&_lastPersistTime);

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
            persist();
        }
    }
    
    // if we were asked to debugTimestampNow do that now...
    if (_debugTimestampNow) {
        quint64 now = usecTimestampNow();
        quint64 sinceLastDebug = now - _lastTimeDebug;
        quint64 DEBUG_TIMESTAMP_INTERVAL = 600000000; // every 10 minutes

        if (sinceLastDebug > DEBUG_TIMESTAMP_INTERVAL) {
            _lastTimeDebug = usecTimestampNow(true); // ask for debug output
        }
        
    }
    return isStillRunning();  // keep running till they terminate us
}


void OctreePersistThread::aboutToFinish() {
    qDebug() << "Persist thread about to finish...";
    persist();
    qDebug() << "Persist thread done with about to finish...";
}

void OctreePersistThread::persist() {
    if (_tree->isDirty()) {
        _tree->lockForWrite();
        {
            qDebug() << "pruning Octree before saving...";
            _tree->pruneTree();
            qDebug() << "DONE pruning Octree before saving...";
        }
        _tree->unlock();

        backup(); // handle backup if requested        

        qDebug() << "saving Octree to file " << _filename << "...";
        _tree->writeToSVOFile(qPrintable(_filename));
        time(&_lastPersistTime);
        _tree->clearDirtyBit(); // tree is clean after saving
        qDebug() << "DONE saving Octree to file...";
    }
}

void OctreePersistThread::backup() {
    if (_wantBackup) {
        quint64 now = usecTimestampNow();
        quint64 sinceLastBackup = now - _lastBackup;
        quint64 MSECS_TO_USECS = 1000;
        quint64 intervalToBackup = _backupInterval * MSECS_TO_USECS;
        
        if (sinceLastBackup > intervalToBackup) {
            struct tm* localTime = localtime(&_lastPersistTime);

            char backupExtension[256];
            strftime(backupExtension, sizeof(backupExtension), qPrintable(_backupExtensionFormat), localTime);

            QString backupFileName = _filename + backupExtension;

            qDebug() << "backing up persist file " << _filename << "to" << backupFileName << "...";
            int result = rename(qPrintable(_filename), qPrintable(backupFileName));
            if (result == 0) {
                qDebug() << "DONE backing up persist file...";
            } else {
                qDebug() << "ERROR in backing up persist file...";
            }
        }
    }
}
