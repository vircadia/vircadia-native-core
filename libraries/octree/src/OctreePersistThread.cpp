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
#include "OctreePersistThread.h"

const int OctreePersistThread::DEFAULT_PERSIST_INTERVAL = 1000 * 30; // every 30 seconds
const QString OctreePersistThread::REPLACEMENT_FILE_EXTENSION = ".replace";

OctreePersistThread::OctreePersistThread(OctreePointer tree, const QString& filename, const QString& backupDirectory, int persistInterval,
                                         bool wantBackup, const QJsonObject& settings, bool debugTimestampNow,
                                         QString persistAsFileType) :
    _tree(tree),
    _filename(filename),
    _backupDirectory(backupDirectory),
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

void OctreePersistThread::parseSettings(const QJsonObject& settings) {
    if (settings["backups"].isArray()) {
        const QJsonArray& backupRules = settings["backups"].toArray();
        qCDebug(octree) << "BACKUP RULES:";

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

            qCDebug(octree) << "    Name:" << obj["Name"].toString();
            qCDebug(octree) << "        format:" << obj["format"].toString();
            qCDebug(octree) << "        interval:" << interval;
            qCDebug(octree) << "        count:" << count;

            BackupRule newRule = { obj["Name"].toString(), interval, obj["format"].toString(), count, 0};
                                    
            newRule.lastBackup = getMostRecentBackupTimeInUsecs(obj["format"].toString());
            
            if (newRule.lastBackup > 0) {
                quint64 now = usecTimestampNow();
                quint64 sinceLastBackup = now - newRule.lastBackup;
                qCDebug(octree) << "        lastBackup:" << qPrintable(formatUsecTime(sinceLastBackup)) << "ago";
            } else {
                qCDebug(octree) << "        lastBackup: NEVER";
            }
            
            _backupRules << newRule;
        }        
    } else {
        qCDebug(octree) << "BACKUP RULES: NONE";
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

void OctreePersistThread::possiblyReplaceContent() {
    // before we load the normal file, check if there's a pending replacement file
    auto replacementFileName = _filename + REPLACEMENT_FILE_EXTENSION;

    QFile replacementFile { replacementFileName };
    if (replacementFile.exists()) {
        // we have a replacement file to process
        qDebug() << "Replacing models file with" << replacementFileName;

        // first take the current models file and move it to a different filename, appended with the timestamp
        QFile currentFile { _filename };
        if (currentFile.exists()) {
            static const QString FILENAME_TIMESTAMP_FORMAT = "yyyyMMdd-hhmmss";
            auto backupFileName = _filename + ".backup." + QDateTime::currentDateTime().toString(FILENAME_TIMESTAMP_FORMAT);

            if (currentFile.rename(backupFileName)) {
                qDebug() << "Moved previous models file to" << backupFileName;
            } else {
                qWarning() << "Could not backup previous models file to" << backupFileName << "- removing replacement models file";

                if (!replacementFile.remove()) {
                    qWarning() << "Could not remove replacement models file from" << replacementFileName
                        << "- replacement will be re-attempted on next server restart";
                    return;
                }
            }
        }

        // rename the replacement file to match what the persist thread is just about to read
        if (!replacementFile.rename(_filename)) {
            qWarning() << "Could not replace models file with" << replacementFileName << "- starting with empty models file";
        }
    }
}


bool OctreePersistThread::process() {

    if (!_initialLoadComplete) {
        possiblyReplaceContent();

        quint64 loadStarted = usecTimestampNow();
        qCDebug(octree) << "loading Octrees from file: " << _filename << "...";

        bool persistantFileRead;

        _tree->withWriteLock([&] {
            PerformanceWarning warn(true, "Loading Octree File", true);

            // First check to make sure "lock" file doesn't exist. If it does exist, then
            // our last save crashed during the save, and we want to load our most recent backup.
            QString lockFileName = _filename + ".lock";
            std::ifstream lockFile(qPrintable(lockFileName), std::ios::in | std::ios::binary | std::ios::ate);
            if (lockFile.is_open()) {
                qCDebug(octree) << "WARNING: Octree lock file detected at startup:" << lockFileName
                    << "-- Attempting to restore from previous backup file.";

                // This is where we should attempt to find the most recent backup and restore from
                // that file as our persist file.
                restoreFromMostRecentBackup();

                lockFile.close();
                qCDebug(octree) << "Loading Octree... lock file closed:" << lockFileName;
                remove(qPrintable(lockFileName));
                qCDebug(octree) << "Loading Octree... lock file removed:" << lockFileName;
            }

            persistantFileRead = _tree->readFromFile(qPrintable(_filename.toLocal8Bit()));
            _tree->pruneTree();
        });

        quint64 loadDone = usecTimestampNow();
        _loadTimeUSecs = loadDone - loadStarted;

        _tree->clearDirtyBit(); // the tree is clean since we just loaded it
        qCDebug(octree, "DONE loading Octrees from file... fileRead=%s", debug::valueOf(persistantFileRead));

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
        
        // This last persist time is not really used until the file is actually persisted. It is only
        // used in formatting the backup filename in cases of non-rolling backup names. However, we don't
        // want an uninitialized value for this, so we set it to the current time (startup of the server)
        time(&_lastPersistTime);

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

        qCDebug(octree) << "persist operation calling backup...";
        backup(); // handle backup if requested        
        qCDebug(octree) << "persist operation DONE with backup...";


        // create our "lock" file to indicate we're saving.
        QString lockFileName = _filename + ".lock";
        std::ofstream lockFile(qPrintable(lockFileName), std::ios::out|std::ios::binary);
        if(lockFile.is_open()) {
            qCDebug(octree) << "saving Octree lock file created at:" << lockFileName;

            _tree->writeToFile(qPrintable(_filename), NULL, _persistAsFileType);
            time(&_lastPersistTime);
            _tree->clearDirtyBit(); // tree is clean after saving
            qCDebug(octree) << "DONE saving Octree to file...";

            lockFile.close();
            qCDebug(octree) << "saving Octree lock file closed:" << lockFileName;
            remove(qPrintable(lockFileName));
            qCDebug(octree) << "saving Octree lock file removed:" << lockFileName;
        }
    }
}

void OctreePersistThread::restoreFromMostRecentBackup() {
    qCDebug(octree) << "Restoring from most recent backup...";
    
    QString mostRecentBackupFileName;
    QDateTime mostRecentBackupTime;
    
    bool recentBackup = getMostRecentBackup(QString(""), mostRecentBackupFileName, mostRecentBackupTime);

    // If we found a backup file, restore from that file.
    if (recentBackup) {
        qCDebug(octree) << "BEST backup file:" << mostRecentBackupFileName << " last modified:" << mostRecentBackupTime.toString();

        qCDebug(octree) << "Removing old file:" << _filename;
        remove(qPrintable(_filename));

        qCDebug(octree) << "Restoring backup file " << mostRecentBackupFileName << "...";
        bool result = QFile::copy(mostRecentBackupFileName, _filename);
        if (result) {
            qCDebug(octree) << "DONE restoring backup file " << mostRecentBackupFileName << "to" << _filename << "...";
        } else {
            qCDebug(octree) << "ERROR while restoring backup file " << mostRecentBackupFileName << "to" << _filename << "...";
            perror("ERROR while restoring backup file");
        }
    } else {
        qCDebug(octree) << "NO BEST backup file found.";
    }
}

bool OctreePersistThread::getMostRecentBackup(const QString& format, 
                            QString& mostRecentBackupFileName, QDateTime& mostRecentBackupTime) {

    // Based on our backup file name, determine the path and file name pattern for backup files
    QFileInfo persistFileInfo(_filename);
    QString path = _backupDirectory;
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
            qCDebug(octree) << "Rolling old backup versions for rule" << rule.name << "...";

            QString backupFileName = _backupDirectory + "/" + QUrl(_filename).fileName();

            // Delete maximum rolling file because rename() fails on Windows if target exists
            QString backupMaxExtensionN = rule.extensionFormat;
            backupMaxExtensionN.replace(QString("%N"), QString::number(rule.maxBackupVersions));
            QString backupMaxFilenameN = backupFileName + backupMaxExtensionN;
            QFile backupMaxFileN(backupMaxFilenameN);
            if (backupMaxFileN.exists()) {
                int result = remove(qPrintable(backupMaxFilenameN));
                if (result != 0) {
                    qCDebug(octree) << "ERROR deleting old backup file " << backupMaxFilenameN;
                }
            }

            for(int n = rule.maxBackupVersions - 1; n > 0; n--) {
                QString backupExtensionN = rule.extensionFormat;
                QString backupExtensionNplusOne = rule.extensionFormat;
                backupExtensionN.replace(QString("%N"), QString::number(n));
                backupExtensionNplusOne.replace(QString("%N"), QString::number(n+1));

                QString backupFilenameN = findMostRecentFileExtension(backupFileName, PERSIST_EXTENSIONS) + backupExtensionN;
                QString backupFilenameNplusOne = backupFileName + backupExtensionNplusOne;

                QFile backupFileN(backupFilenameN);

                if (backupFileN.exists()) {
                    qCDebug(octree) << "rolling backup file " << backupFilenameN << "to" << backupFilenameNplusOne << "...";
                    int result = rename(qPrintable(backupFilenameN), qPrintable(backupFilenameNplusOne));
                    if (result == 0) {
                        qCDebug(octree) << "DONE rolling backup file " << backupFilenameN << "to" << backupFilenameNplusOne << "...";
                    } else {
                        qCDebug(octree) << "ERROR in rolling backup file " << backupFilenameN << "to" << backupFilenameNplusOne << "...";
                        perror("ERROR in rolling backup file");
                    }
                }
            }
            qCDebug(octree) << "Done rolling old backup versions...";
        } else {
            qCDebug(octree) << "Rolling backups for rule" << rule.name << "."
                     << " Max Rolled Backup Versions less than 1 [" << rule.maxBackupVersions << "]."
                     << " No need to roll backups...";
        }
    }
}


void OctreePersistThread::backup() {
    qCDebug(octree) << "backup operation wantBackup:" << _wantBackup;
    if (_wantBackup) {
        quint64 now = usecTimestampNow();
        
        for(int i = 0; i < _backupRules.count(); i++) {
            BackupRule& rule = _backupRules[i];

            quint64 sinceLastBackup = now - rule.lastBackup;
            quint64 SECS_TO_USECS = 1000 * 1000;
            quint64 intervalToBackup = rule.interval * SECS_TO_USECS;

            qCDebug(octree) << "Checking [" << rule.name << "] - Time since last backup [" << sinceLastBackup << "] " << 
                        "compared to backup interval [" << intervalToBackup << "]...";

            if (sinceLastBackup > intervalToBackup) {
                qCDebug(octree) << "Time since last backup [" << sinceLastBackup << "] for rule [" << rule.name 
                                        << "] exceeds backup interval [" << intervalToBackup << "] doing backup now...";

                struct tm* localTime = localtime(&_lastPersistTime);

                QString backupFileName = _backupDirectory + "/" + QUrl(_filename).fileName();
            
                // check to see if they asked for version rolling format
                if (rule.extensionFormat.contains("%N")) {
                    rollOldBackupVersions(rule); // rename all the old backup files accordingly
                    QString backupExtension = rule.extensionFormat;
                    backupExtension.replace(QString("%N"), QString("1"));
                    backupFileName += backupExtension;
                } else {
                    char backupExtension[256];
                    strftime(backupExtension, sizeof(backupExtension), qPrintable(rule.extensionFormat), localTime);
                    backupFileName += backupExtension;
                }

                if (rule.maxBackupVersions > 0) {
                    QFile persistFile(_filename);
                    if (persistFile.exists()) {
                        qCDebug(octree) << "backing up persist file " << _filename << "to" << backupFileName << "...";
                        bool result = QFile::copy(_filename, backupFileName);
                        if (result) {
                            qCDebug(octree) << "DONE backing up persist file...";
                            rule.lastBackup = now; // only record successful backup in this case.
                        } else {
                            qCDebug(octree) << "ERROR in backing up persist file...";
                            perror("ERROR in backing up persist file");
                        }
                    } else {
                        qCDebug(octree) << "persist file " << _filename << " does not exist. " << 
                                    "nothing to backup for this rule ["<< rule.name << "]...";
                    }
                } else {
                    qCDebug(octree) << "This backup rule" << rule.name 
                             << " has Max Rolled Backup Versions less than 1 [" << rule.maxBackupVersions << "]."
                             << " There are no backups to be done...";
                }
            } else {
                qCDebug(octree) << "Backup not needed for this rule ["<< rule.name << "]...";
            }
        }
    }
}
