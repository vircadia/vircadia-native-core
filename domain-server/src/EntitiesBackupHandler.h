//
//  EntitiesBackupHandler.h
//  domain-server/src
//
//  Created by Clement Brisset on 2/14/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntitiesBackupHandler_h
#define hifi_EntitiesBackupHandler_h

#include "BackupHandler.h"

class EntitiesBackupHandler : public BackupHandlerInterface {
public:
    EntitiesBackupHandler(QString entitiesFilePath, QString entitiesReplacementFilePath);

    std::pair<bool, float> isAvailable(const QString& backupName) override { return { true, 1.0f }; }
    std::pair<bool, float> getRecoveryStatus() override { return { false, 1.0f }; }

    void loadBackup(const QString& backupName, QuaZip& zip) override {}

    void loadingComplete() override {}

    // Create a skeleton backup
    void createBackup(const QString& backupName, QuaZip& zip) override;

    // Recover from a full backup
    std::pair<bool, QString> recoverBackup(const QString& backupName, QuaZip& zip, const QString& username, const QString& sourceFilename) override;

    // Delete a skeleton backup
    void deleteBackup(const QString& backupName) override {}

    // Create a full backup
    void consolidateBackup(const QString& backupName, QuaZip& zip) override {}

    bool isCorruptedBackup(const QString& backupName) override { return false; }

private:
    QString _entitiesFilePath;
    QString _entitiesReplacementFilePath;
};

#endif /* hifi_EntitiesBackupHandler_h */
