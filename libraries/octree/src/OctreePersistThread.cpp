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

#include <PerfStat.h>
#include <SharedUtil.h>

#include "OctreePersistThread.h"

const int OctreePersistThread::DEFAULT_PERSIST_INTERVAL = 1000 * 30; // every 30 seconds

OctreePersistThread::OctreePersistThread(Octree* tree, const QString& filename, int persistInterval, 
                                         bool wantBackup, const QJsonObject& settings, bool debugTimestampNow,
                                         QString persistAsFileType) :
    _tree(tree),
    _filename(filename),
    _persistInterval(persistInterval),
    _initialLoadComplete(false),
    _loadTimeUSecs(0),
    _lastCheck(0),
    _wantBackup(wantBackup),
    _debugTimestampNow(debugTimestampNow),
    _lastTimeDebug(0),
    _persistAsFileType(persistAsFileType)
{
    parseSettings(settings);

    // in case the persist filename has an extension that doesn't match the file type
    QString sansExt = fileNameWithoutExtension(_filename, persistExtensions);
    _filename = sansExt + "." + _persistAsFileType;
}

void OctreePersistThread::parseSettings(const QJsonObject& settings) {
    if (settings["backups"].isArray()) {
        const QJsonArray& backupRules = settings["backups"].toArray();
        qDebug() << "BACKUP RULES:";

        foreach (const QJsonValue& value, backupRules) {

            QJsonObject obj = value.toObject();

            int interval = 0;
            int count = 0;
            
            QJsonValue intervalVal = obj["backupInterval"];
            if (intervalVal.isString()) {
                interval = intervalVal.toString().toInt();
            } else {
                interval = intervalVal.toInt();
            }

            QJsonValue countVal = obj["maxBackupVersions"];
            if (countVal.isString()) {
                count = countVal.toString().toInt();
            } else {
                count = countVal.toInt();
            }

            qDebug() << "    Name:" << obj["Name"].toString();
            qDebug() << "        format:" << obj["format"].toString();
            qDebug() << "        interval:" << interval;
            qDebug() << "        count:" << count;

            BackupRule newRule = { obj["Name"].toString(), interval, obj["format"].toString(), count, 0};
                                    
            newRule.lastBackup = getMostRecentBackupTimeInUsecs(obj["format"].toString());
            
            if (newRule.lastBackup > 0) {
                quint64 now = usecTimestampNow();
                quint64 sinceLastBackup = now - newRule.lastBackup;
                qDebug() << "        lastBackup:" << qPrintable(formatUsecTime(sinceLastBackup)) << "ago";
            } else {
                qDebug() << "        lastBackup: NEVER";
            }
            
            _backupRules << newRule;
        }        
    } else {
        qDebug() << "BACKUP RULES: NONE";
    }
}

quint64 OctreePersistThread::getMostRecentBackupTimeInUsecs(const QString& format) {

    quint64 mostRecentBackupInUsecs = 0;

    QString mostRecentBackupFileName;
    QDateTime mostRecentBackupTime;
    
    bool recentBackup = getMostRecentBackup(format, mostRecentBackupFileName, mostRecentBackupTime);
    
    if (recentBackup) {
        mostRecentBackupInUsecs = mostRecentBackupTime.toMSecsSinceEpoch() * USECS_PER_MSEC;
    }

    return mostRecentBackupInUsecs;
}


bool OctreePersistThread::process() {

    if (!_initialLoadComplete) {
        quint64 loadStarted = usecTimestampNow();
        qDebug() << "loading Octrees from file: " << _filename << "...";

        bool persistantFileRead;

        _tree->lockForWrite();
        {
            PerformanceWarning warn(true, "Loading Octree File", true);
            
            // First check to make sure "lock" file doesn't exist. If it does exist, then
            // our last save crashed during the save, and we want to load our most recent backup.
            QString lockFileName = _filename + ".lock";
            std::ifstream lockFile(qPrintable(lockFileName), std::ios::in|std::ios::binary|std::ios::ate);
            if(lockFile.is_open()) {
                qDebug() << "WARNING: Octree lock file detected at startup:" << lockFileName
                         << "-- Attempting to restore from previous backup file.";
                         
                // This is where we should attempt to find the most recent backup and restore from
                // that file as our persist file.
                restoreFromMostRecentBackup();

                lockFile.close();
                qDebug() << "Loading Octree... lock file closed:" << lockFileName;
                remove(qPrintable(lockFileName));
                qDebug() << "Loading Octree... lock file removed:" << lockFileName;
            }

            persistantFileRead = _tree->readFromFile(_filename.toLocal8Bit().constData());
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

        // Since we just loaded the persistent file, we can consider ourselves as having "just checked" for persistance.
        _lastCheck = usecTimestampNow(); // we just loaded, no need to save again
        
        // This last persist time is not really used until the file is actually persisted. It is only
        // used in formatting the backup filename in cases of non-rolling backup names. However, we don't
        // want an uninitialized value for this, so we set it to the current time (startup of the server)
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

        qDebug() << "persist operation calling backup...";
        backup(); // handle backup if requested        
        qDebug() << "persist operation DONE with backup...";


        // create our "lock" file to indicate we're saving.
        QString lockFileName = _filename + ".lock";
        std::ofstream lockFile(qPrintable(lockFileName), std::ios::out|std::ios::binary);
        if(lockFile.is_open()) {
            qDebug() << "saving Octree lock file created at:" << lockFileName;

            _tree->writeToFile(qPrintable(_filename), NULL, _persistAsFileType);
            time(&_lastPersistTime);
            _tree->clearDirtyBit(); // tree is clean after saving
            qDebug() << "DONE saving Octree to file...";

            lockFile.close();
            qDebug() << "saving Octree lock file closed:" << lockFileName;
            remove(qPrintable(lockFileName));
            qDebug() << "saving Octree lock file removed:" << lockFileName;
        }
    }
}

void OctreePersistThread::restoreFromMostRecentBackup() {
    qDebug() << "Restoring from most recent backup...";
    
    QString mostRecentBackupFileName;
    QDateTime mostRecentBackupTime;
    
    bool recentBackup = getMostRecentBackup(QString(""), mostRecentBackupFileName, mostRecentBackupTime);

    // If we found a backup file, restore from that file.
    if (recentBackup) {
        qDebug() << "BEST backup file:" << mostRecentBackupFileName << " last modified:" << mostRecentBackupTime.toString();

        qDebug() << "Removing old file:" << _filename;
        remove(qPrintable(_filename));

        qDebug() << "Restoring backup file " << mostRecentBackupFileName << "...";
        bool result = QFile::copy(mostRecentBackupFileName, _filename);
        if (result) {
            qDebug() << "DONE restoring backup file " << mostRecentBackupFileName << "to" << _filename << "...";
        } else {
            qDebug() << "ERROR while restoring backup file " << mostRecentBackupFileName << "to" << _filename << "...";
        }
    } else {
        qDebug() << "NO BEST backup file found.";
    }
}

bool OctreePersistThread::getMostRecentBackup(const QString& format, 
                            QString& mostRecentBackupFileName, QDateTime& mostRecentBackupTime) {

    // Based on our backup file name, determine the path and file name pattern for backup files
    QFileInfo persistFileInfo(_filename);
    QString path = persistFileInfo.path();
    QString fileNamePart = persistFileInfo.fileName();

    QStringList filters;

    if (format.isEmpty()) {
        // Create a file filter that will find all backup files of this extension format
        foreach(const BackupRule& rule, _backupRules) {
            QString backupExtension = rule.extensionFormat;
            backupExtension.replace(QRegExp("%."), "*");
            QString backupFileNamePart = fileNamePart + backupExtension;
            filters << backupFileNamePart;
        }
    } else {
        QString backupExtension = format;
        backupExtension.replace(QRegExp("%."), "*");
        QString backupFileNamePart = fileNamePart + backupExtension;
        filters << backupFileNamePart;
    }
    
    bool bestBackupFound = false;        
    QString bestBackupFile;
    QDateTime bestBackupFileTime;

    // Iterate over all of the backup files in the persist location
    QDirIterator dirIterator(path, filters, QDir::Files|QDir::NoSymLinks, QDirIterator::NoIteratorFlags);
    while(dirIterator.hasNext()) {

        dirIterator.next();
        QDateTime lastModified = dirIterator.fileInfo().lastModified();

        // Based on last modified date, track the most recently modified file as the best backup
        if (lastModified > bestBackupFileTime) {
            bestBackupFound = true;
            bestBackupFile = dirIterator.filePath();
            bestBackupFileTime = lastModified;
        }
    }

    // If we found a backup then return the results    
    if (bestBackupFound) {
        mostRecentBackupFileName = bestBackupFile;
        mostRecentBackupTime = bestBackupFileTime;
    }
    return bestBackupFound;
}

void OctreePersistThread::rollOldBackupVersions(const BackupRule& rule) {

    if (rule.extensionFormat.contains("%N")) {
        if (rule.maxBackupVersions > 0) {
            qDebug() << "Rolling old backup versions for rule" << rule.name << "...";
            for(int n = rule.maxBackupVersions - 1; n > 0; n--) {
                QString backupExtensionN = rule.extensionFormat;
                QString backupExtensionNplusOne = rule.extensionFormat;
                backupExtensionN.replace(QString("%N"), QString::number(n));
                backupExtensionNplusOne.replace(QString("%N"), QString::number(n+1));

                QString backupFilenameN = findMostRecentPersist(_filename) + backupExtensionN;
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
        } else {
            qDebug() << "Rolling backups for rule" << rule.name << "."
                     << " Max Rolled Backup Versions less than 1 [" << rule.maxBackupVersions << "]."
                     << " No need to roll backups...";
        }
    }
}


void OctreePersistThread::backup() {
    qDebug() << "backup operation wantBackup:" << _wantBackup;
    if (_wantBackup) {
        quint64 now = usecTimestampNow();
        
        for(int i = 0; i < _backupRules.count(); i++) {
            BackupRule& rule = _backupRules[i];

            quint64 sinceLastBackup = now - rule.lastBackup;
            quint64 SECS_TO_USECS = 1000 * 1000;
            quint64 intervalToBackup = rule.interval * SECS_TO_USECS;

            qDebug() << "Checking [" << rule.name << "] - Time since last backup [" << sinceLastBackup << "] " << 
                        "compared to backup interval [" << intervalToBackup << "]...";

            if (sinceLastBackup > intervalToBackup) {
                qDebug() << "Time since last backup [" << sinceLastBackup << "] for rule [" << rule.name 
                                        << "] exceeds backup interval [" << intervalToBackup << "] doing backup now...";

                struct tm* localTime = localtime(&_lastPersistTime);

                QString backupFileName;
            
                // check to see if they asked for version rolling format
                if (rule.extensionFormat.contains("%N")) {
                    rollOldBackupVersions(rule); // rename all the old backup files accordingly
                    QString backupExtension = rule.extensionFormat;
                    backupExtension.replace(QString("%N"), QString("1"));
                    backupFileName = _filename + backupExtension;
                } else {
                    char backupExtension[256];
                    strftime(backupExtension, sizeof(backupExtension), qPrintable(rule.extensionFormat), localTime);
                    backupFileName = _filename + backupExtension;
                }


                if (rule.maxBackupVersions > 0) {
                    QFile persistFile(_filename);
                    if (persistFile.exists()) {
                        qDebug() << "backing up persist file " << _filename << "to" << backupFileName << "...";
                        bool result = QFile::copy(_filename, backupFileName);
                        if (result) {
                            qDebug() << "DONE backing up persist file...";
                            rule.lastBackup = now; // only record successful backup in this case.
                        } else {
                            qDebug() << "ERROR in backing up persist file...";
                        }
                    } else {
                        qDebug() << "persist file " << _filename << " does not exist. " << 
                                    "nothing to backup for this rule ["<< rule.name << "]...";
                    }
                } else {
                    qDebug() << "This backup rule" << rule.name 
                             << " has Max Rolled Backup Versions less than 1 [" << rule.maxBackupVersions << "]."
                             << " There are no backups to be done...";
                }
            } else {
                qDebug() << "Backup not needed for this rule ["<< rule.name << "]...";
            }
        }
    }
}
