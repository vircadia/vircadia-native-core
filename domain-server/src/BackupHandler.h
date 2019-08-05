//
//  BackupHandler.h
//  domain-server/src
//
//  Created by Clement Brisset on 2/5/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BackupHandler_h
#define hifi_BackupHandler_h

#include <memory>

#include <QString>

class QuaZip;

class BackupHandlerInterface {
public:
    virtual ~BackupHandlerInterface() = default;

    virtual std::pair<bool, float> isAvailable(const QString& backupName) = 0;

    // Returns whether a recovery is ongoing and a progress between 0 and 1 if one is.
    virtual std::pair<bool, float> getRecoveryStatus() = 0;

    virtual void loadBackup(const QString& backupName, QuaZip& zip) = 0;
    virtual void loadingComplete() = 0;
    virtual void createBackup(const QString& backupName, QuaZip& zip) = 0;
    virtual std::pair<bool, QString> recoverBackup(const QString& backupName, QuaZip& zip, const QString& username, const QString& sourceFilename) = 0;
    virtual void deleteBackup(const QString& backupName) = 0;
    virtual void consolidateBackup(const QString& backupName, QuaZip& zip) = 0;
    virtual bool isCorruptedBackup(const QString& backupName) = 0;
};
using BackupHandlerPointer = std::unique_ptr<BackupHandlerInterface>;

#endif /* hifi_BackupHandler_h */
