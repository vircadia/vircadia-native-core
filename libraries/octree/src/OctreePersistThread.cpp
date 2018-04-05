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

#include "OctreePersistThread.h"

#include <chrono>
#include <thread>

#include <cstdio>
#include <fstream>
#include <time.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include <NumericalConstants.h>
#include <PerfStat.h>
#include <PathUtils.h>

#include "OctreeLogging.h"
#include "OctreeUtils.h"
#include "OctreeDataUtils.h"

const int OctreePersistThread::DEFAULT_PERSIST_INTERVAL = 1000 * 30; // every 30 seconds

OctreePersistThread::OctreePersistThread(OctreePointer tree, const QString& filename, int persistInterval,
                                         bool debugTimestampNow, QString persistAsFileType, const QByteArray& replacementData) :
    _tree(tree),
    _filename(filename),
    _persistInterval(persistInterval),
    _initialLoadComplete(false),
    _replacementData(replacementData),
    _loadTimeUSecs(0),
    _lastCheck(0),
    _debugTimestampNow(debugTimestampNow),
    _lastTimeDebug(0),
    _persistAsFileType(persistAsFileType)
{
    // in case the persist filename has an extension that doesn't match the file type
    QString sansExt = fileNameWithoutExtension(_filename, PERSIST_EXTENSIONS);
    _filename = sansExt + "." + _persistAsFileType;
}

QString OctreePersistThread::getPersistFileMimeType() const {
    if (_persistAsFileType == "json") {
        return "application/json";
    } if (_persistAsFileType == "json.gz") {
        return "application/zip";
    }
    return "";
}

void OctreePersistThread::replaceData(QByteArray data) {
    backupCurrentFile();

    QFile currentFile { _filename };
    if (currentFile.open(QIODevice::WriteOnly)) {
        currentFile.write(data);
        qDebug() << "Wrote replacement data";
    } else {
        qWarning() << "Failed to write replacement data";
    }
}

// Return true if current file is backed up successfully or doesn't exist.
bool OctreePersistThread::backupCurrentFile() {
    // first take the current models file and move it to a different filename, appended with the timestamp
    QFile currentFile { _filename };
    if (currentFile.exists()) {
        static const QString FILENAME_TIMESTAMP_FORMAT = "yyyyMMdd-hhmmss";
        auto backupFileName = _filename + ".backup." + QDateTime::currentDateTime().toString(FILENAME_TIMESTAMP_FORMAT);

        if (currentFile.rename(backupFileName)) {
            qDebug() << "Moved previous models file to" << backupFileName;
            return true;
        } else {
            qWarning() << "Could not backup previous models file to" << backupFileName << "- removing replacement models file";
            return false;
        }
    }
    return true;
}

bool OctreePersistThread::process() {

    if (!_initialLoadComplete) {
        quint64 loadStarted = usecTimestampNow();
        qCDebug(octree) << "loading Octrees from file: " << _filename << "...";

        if (!_replacementData.isNull()) {
            replaceData(_replacementData);
        }

        OctreeUtils::RawOctreeData data;
        if (data.readOctreeDataInfoFromFile(_filename)) {
            qDebug() << "Setting entity version info to: " << data.id << data.dataVersion;
            _tree->setOctreeVersionInfo(data.id, data.dataVersion);
        }

        bool persistentFileRead;

        _tree->withWriteLock([&] {
            PerformanceWarning warn(true, "Loading Octree File", true);

            persistentFileRead = _tree->readFromFile(_filename.toLocal8Bit().constData());
            _tree->pruneTree();
        });

        quint64 loadDone = usecTimestampNow();
        _loadTimeUSecs = loadDone - loadStarted;

        _tree->clearDirtyBit(); // the tree is clean since we just loaded it
        qCDebug(octree, "DONE loading Octrees from file... fileRead=%s", debug::valueOf(persistentFileRead));

        unsigned long nodeCount = OctreeElement::getNodeCount();
        unsigned long internalNodeCount = OctreeElement::getInternalNodeCount();
        unsigned long leafNodeCount = OctreeElement::getLeafNodeCount();
        qCDebug(octree, "Nodes after loading scene %lu nodes %lu internal %lu leaves", nodeCount, internalNodeCount, leafNodeCount);

        bool wantDebug = false;
        if (wantDebug) {
            double usecPerGet = (double)OctreeElement::getGetChildAtIndexTime() 
                                    / (double)OctreeElement::getGetChildAtIndexCalls();
            qCDebug(octree) << "getChildAtIndexCalls=" << OctreeElement::getGetChildAtIndexCalls()
                    << " getChildAtIndexTime=" << OctreeElement::getGetChildAtIndexTime() << " perGet=" << usecPerGet;

            double usecPerSet = (double)OctreeElement::getSetChildAtIndexTime() 
                                    / (double)OctreeElement::getSetChildAtIndexCalls();
            qCDebug(octree) << "setChildAtIndexCalls=" << OctreeElement::getSetChildAtIndexCalls()
                    << " setChildAtIndexTime=" << OctreeElement::getSetChildAtIndexTime() << " perSet=" << usecPerSet;
        }

        _initialLoadComplete = true;

        // Since we just loaded the persistent file, we can consider ourselves as having "just checked" for persistance.
        _lastCheck = usecTimestampNow(); // we just loaded, no need to save again

        if (_replacementData.isNull()) {
            sendLatestEntityDataToDS();
        }
        _replacementData.clear();

        emit loadCompleted();
    }

    if (isStillRunning()) {
        quint64 MSECS_TO_USECS = 1000;
        quint64 USECS_TO_SLEEP = 10 * MSECS_TO_USECS; // every 10ms
        std::this_thread::sleep_for(std::chrono::microseconds(USECS_TO_SLEEP));

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
    qCDebug(octree) << "Persist thread about to finish...";
    persist();
    qCDebug(octree) << "Persist thread done with about to finish...";
    _stopThread = true;
}

QByteArray OctreePersistThread::getPersistFileContents() const {
    QByteArray fileContents;
    QFile file(_filename);
    if (file.open(QIODevice::ReadOnly)) {
        fileContents = file.readAll();
    }
    return fileContents;
}

void OctreePersistThread::persist() {
    if (_tree->isDirty() && _initialLoadComplete) {

        _tree->withWriteLock([&] {
            qCDebug(octree) << "pruning Octree before saving...";
            _tree->pruneTree();
            qCDebug(octree) << "DONE pruning Octree before saving...";
        });

        _tree->incrementPersistDataVersion();

        // create our "lock" file to indicate we're saving.
        QString lockFileName = _filename + ".lock";
        std::ofstream lockFile(lockFileName.toLocal8Bit().constData(), std::ios::out|std::ios::binary);
        if(lockFile.is_open()) {
            qCDebug(octree) << "saving Octree lock file created at:" << lockFileName;

            _tree->writeToFile(_filename.toLocal8Bit().constData(), NULL, _persistAsFileType);
            _tree->clearDirtyBit(); // tree is clean after saving
            qCDebug(octree) << "DONE saving Octree to file...";

            lockFile.close();
            qCDebug(octree) << "saving Octree lock file closed:" << lockFileName;
            remove(lockFileName.toLocal8Bit().constData());
            qCDebug(octree) << "saving Octree lock file removed:" << lockFileName;
        }

        sendLatestEntityDataToDS();
    }
}

void OctreePersistThread::sendLatestEntityDataToDS() {
    qDebug() << "Sending latest entity data to DS";
    auto nodeList = DependencyManager::get<NodeList>();
    const DomainHandler& domainHandler = nodeList->getDomainHandler();

    QByteArray data;
    if (_tree->toJSON(&data, nullptr, true)) {
        auto message = NLPacketList::create(PacketType::OctreeDataPersist, QByteArray(), true, true);
        message->write(data);
        nodeList->sendPacketList(std::move(message), domainHandler.getSockAddr());
    } else {
        qCWarning(octree) << "Failed to persist octree to DS";
    }
}
