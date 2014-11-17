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
#include <QFile>

#include <PerfStat.h>
#include <SharedUtil.h>

#include "OctreePersistThread.h"

const int OctreePersistThread::DEFAULT_PERSIST_INTERVAL = 1000 * 30; // every 30 seconds
const int OctreePersistThread::DEFAULT_BACKUP_INTERVAL = 1000 * 60 * 30; // every 30 minutes
const QString OctreePersistThread::DEFAULT_BACKUP_EXTENSION_FORMAT(".backup.%N");
const int OctreePersistThread::DEFAULT_MAX_BACKUP_VERSIONS = 5;


OctreePersistThread::OctreePersistThread(Octree* tree, const QString& filename, int persistInterval, 
                                                bool wantBackup, int backupInterval, const QString& backupExtensionFormat,
                                                int maxBackupVersions) :
    _tree(tree),
    _filename(filename),
    _backupExtensionFormat(backupExtensionFormat),
    _maxBackupVersions(maxBackupVersions),
    _persistInterval(persistInterval),
    _backupInterval(backupInterval),
    _initialLoadComplete(false),
    _loadTimeUSecs(0),
    _lastCheck(0),
    _lastBackup(0),
    _wantBackup(wantBackup)
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

void OctreePersistThread::rollOldBackupVersions() {
    if (!_backupExtensionFormat.contains("%N")) {
        return; // this backup extension format doesn't support rolling
    }

    qDebug() << "Rolling old backup versions...";
    for(int n = _maxBackupVersions - 1; n > 0; n--) {
        QString backupExtensionN = _backupExtensionFormat;
        QString backupExtensionNplusOne = _backupExtensionFormat;
        backupExtensionN.replace(QString("%N"), QString::number(n));
        backupExtensionNplusOne.replace(QString("%N"), QString::number(n+1));
        
        QString backupFilenameN = _filename + backupExtensionN;
        QString backupFilenameNplusOne = _filename + backupExtensionNplusOne;

        QFile backupFileN(backupFilenameN);

        if (backupFileN.exists()) {
            qDebug() << "rolling backup file " << backupFilenameN << "to" << backupFilenameNplusOne << "...";
            int result = rename(qPrintable(backupFilenameN), qPrintable(backupFilenameNplusOne));
            if (result == 0) {
                qDebug() << "DONE rolling backup file " << backupFilenameN << "to" << backupFilenameNplusOne << "...";
            } else {
                qDebug() << "ERROR in rolling backup file " << backupFilenameN << "to" << backupFilenameNplusOne << "...";
            }
        }
    }
    qDebug() << "Done rolling old backup versions...";
}


void OctreePersistThread::backup() {
    if (_wantBackup) {
        quint64 now = usecTimestampNow();
        quint64 sinceLastBackup = now - _lastBackup;
        quint64 MSECS_TO_USECS = 1000;
        quint64 intervalToBackup = _backupInterval * MSECS_TO_USECS;
        
        if (sinceLastBackup > intervalToBackup) {
            struct tm* localTime = localtime(&_lastPersistTime);

            QString backupFileName;
            
            // check to see if they asked for version rolling format
            if (_backupExtensionFormat.contains("%N")) {
                rollOldBackupVersions(); // rename all the old backup files accordingly
                QString backupExtension = _backupExtensionFormat;
                backupExtension.replace(QString("%N"), QString("1"));
                backupFileName = _filename + backupExtension;
            } else {
                char backupExtension[256];
                strftime(backupExtension, sizeof(backupExtension), qPrintable(_backupExtensionFormat), localTime);
                backupFileName = _filename + backupExtension;
            }


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
