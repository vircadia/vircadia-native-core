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
#include <QRegExp>

#include <NumericalConstants.h>
#include <PerfStat.h>
#include <PathUtils.h>

#include "OctreeLogging.h"
#include "OctreeUtils.h"
#include "OctreeDataUtils.h"

const std::chrono::seconds OctreePersistThread::DEFAULT_PERSIST_INTERVAL { 30 };

constexpr int MAX_OCTREE_REPLACEMENT_BACKUP_FILES_COUNT { 20 };
constexpr size_t MAX_OCTREE_REPLACEMENT_BACKUP_FILES_SIZE_BYTES { 50 * 1000 * 1000 };

OctreePersistThread::OctreePersistThread(OctreePointer tree, const QString& filename, std::chrono::milliseconds persistInterval,
                                         bool debugTimestampNow, QString persistAsFileType) :
    _tree(tree),
    _filename(filename),
    _persistInterval(persistInterval),
    _initialLoadComplete(false),
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

void OctreePersistThread::start() {
    cleanupOldReplacementBackups();

    QFile tempFile { getTempFilename() };
    if (tempFile.exists()) {
        qWarning(octree) << "Found temporary octree file at" << tempFile.fileName();
        qDebug(octree) << "Attempting to recover from temporary octree file";
        QFile::remove(_filename);
        if (tempFile.rename(_filename)) {
            qDebug(octree) << "Successfully recovered from temporary octree file";
        } else {
            qWarning(octree) << "Failed to recover from temporary octree file";
            tempFile.remove();
        }
    }

    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(PacketType::OctreeDataFileReply, this, "handleOctreeDataFileReply");

    auto nodeList = DependencyManager::get<NodeList>();
    const DomainHandler& domainHandler = nodeList->getDomainHandler();

    auto packet = NLPacket::create(PacketType::OctreeDataFileRequest, -1, true, false);

    OctreeUtils::RawOctreeData data;
    qCDebug(octree) << "Reading octree data from" << _filename;
    if (data.readOctreeDataInfoFromFile(_filename)) {
        qCDebug(octree) << "Current octree data: ID(" << data.id << ") DataVersion(" << data.version << ")";
        packet->writePrimitive(true);
        auto id = data.id.toRfc4122();
        packet->write(id);
        packet->writePrimitive(data.version);
    } else {
        qCWarning(octree) << "No octree data found";
        packet->writePrimitive(false);
    }

    qCDebug(octree) << "Sending request for octree data to DS";
    nodeList->sendPacket(std::move(packet), domainHandler.getSockAddr());
}

void OctreePersistThread::handleOctreeDataFileReply(QSharedPointer<ReceivedMessage> message) {
    if (_initialLoadComplete) {
        qCWarning(octree) << "Received OctreeDataFileReply after initial load had completed";
        return;
    }

    bool includesNewData;
    message->readPrimitive(&includesNewData);
    QByteArray replacementData;
    if (includesNewData) {
        replacementData = message->readAll();
        replaceData(replacementData);
        qDebug() << "Got reply to octree data file request, new data sent";
    } else {
        qDebug() << "Got reply to octree data file request, current entity data is sufficient";
        
        OctreeUtils::RawEntityData data;
        qCDebug(octree) << "Reading octree data from" << _filename;
        if (data.readOctreeDataInfoFromFile(_filename)) {
            if (data.id.isNull()) {
                qCDebug(octree) << "Current octree data has a null id, updating";
                data.resetIdAndVersion();

                QFile file(_filename);
                if (file.open(QIODevice::WriteOnly)) {
                    auto entityData = data.toGzippedByteArray();
                    file.write(entityData);
                    file.close();
                } else {
                    qCDebug(octree) << "Failed to update octree data";
                }
            }
        }
    }

    quint64 loadStarted = usecTimestampNow();
    qCDebug(octree) << "loading Octrees from file: " << _filename << "...";

    OctreeUtils::RawOctreeData data;
    if (data.readOctreeDataInfoFromFile(_filename)) {
        qDebug() << "Setting entity version info to: " << data.id << data.version;
        _tree->setOctreeVersionInfo(data.id, data.version);
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

    if (replacementData.isNull()) {
        sendLatestEntityDataToDS();
    }

    qDebug() << "Starting timer";
    QTimer::singleShot(std::chrono::milliseconds(_persistInterval).count(), this, &OctreePersistThread::process);
    QTimer::singleShot(std::chrono::milliseconds(1000).count(), this, &OctreePersistThread::process);

    emit loadCompleted();
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
    qDebug() << "Processing...";

    if (true) { //isStillRunning()) {
        quint64 MSECS_TO_USECS = 1000;
        quint64 USECS_TO_SLEEP = 10 * MSECS_TO_USECS; // every 10ms
        std::this_thread::sleep_for(std::chrono::microseconds(USECS_TO_SLEEP));

        // do our updates then check to save...
        _tree->update();

        quint64 now = usecTimestampNow();
        quint64 sinceLastSave = now - _lastCheck;
        quint64 intervalToCheck = std::chrono::microseconds(_persistInterval).count();

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
    //return isStillRunning();  // keep running till they terminate us
    QTimer::singleShot(1000, this, &OctreePersistThread::process);
    return true;
}

void OctreePersistThread::aboutToFinish() {
    qCDebug(octree) << "Persist thread about to finish...";
    persist();
    qCDebug(octree) << "Persist thread done with about to finish...";
}

QByteArray OctreePersistThread::getPersistFileContents() const {
    QByteArray fileContents;
    QFile file(_filename);
    if (file.open(QIODevice::ReadOnly)) {
        fileContents = file.readAll();
    }
    return fileContents;
}

void OctreePersistThread::cleanupOldReplacementBackups() {
    QRegExp filenameRegex { ".*\\.backup\\.\\d{8}-\\d{6}$" };
    QFileInfo persistFile { _filename };
    QDir backupDir { persistFile.absolutePath() };
    backupDir.setSorting(QDir::SortFlag::Time);
    backupDir.setFilter(QDir::Filter::Files);
    qDebug() << "Scanning backups for cleanup:" << backupDir.absolutePath();

    auto count = 0;
    size_t totalSize = 0;
    for (auto fileInfo : backupDir.entryInfoList()) {
        auto absPath = fileInfo.absoluteFilePath();
        qDebug() << "Found:" << absPath;
        if (filenameRegex.exactMatch(absPath)) {
            totalSize += fileInfo.size();
            if (count >= MAX_OCTREE_REPLACEMENT_BACKUP_FILES_COUNT || totalSize > MAX_OCTREE_REPLACEMENT_BACKUP_FILES_SIZE_BYTES) {
                qDebug() << "Removing:" << absPath;
                QFile backup(absPath);
                if (backup.remove()) {
                    qDebug() << "Removed backup:" << absPath;
                } else {
                    qWarning() << "Failed to remove backup:" << absPath;

                }
            }
            count++;
        }
    }
    qDebug() << "Found" << count << "backups";
}

void OctreePersistThread::persist() {
    if (_tree->isDirty() && _initialLoadComplete) {

        _tree->withWriteLock([&] {
            qCDebug(octree) << "pruning Octree before saving...";
            _tree->pruneTree();
            qCDebug(octree) << "DONE pruning Octree before saving...";
        });

        _tree->incrementPersistDataVersion();

        QString tempFilename = getTempFilename();
        qCDebug(octree) << "Saving temporary Octree file to:" << tempFilename;
        if (_tree->writeToFile(tempFilename.toLocal8Bit().constData(), nullptr, _persistAsFileType)) {
            QFile tempFile { tempFilename };
            _tree->clearDirtyBit(); // tree is clean after saving
            QFile::remove(_filename);
            if (tempFile.rename(_filename)) {
                qCDebug(octree) << "DONE moving temporary Octree file to" << _filename;
            } else {
                qCWarning(octree) << "Failed to move temporary Octree file to " << _filename;
                tempFile.remove();
            }
        } else {
            qCWarning(octree) << "Failed to open temp Octree file at" << tempFilename;
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
