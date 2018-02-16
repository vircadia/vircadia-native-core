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

    std::pair<bool, float> isAvailable(QString filePath) override { return { true, 1.0f }; }
    std::pair<bool, float> getRecoveryStatus() override { return { false, 1.0f }; }

    void loadBackup(QuaZip& zip) override {}

    // Create a skeleton backup
    void createBackup(QuaZip& zip) override;

    // Recover from a full backup
    void recoverBackup(QuaZip& zip) override;

    // Delete a skeleton backup
    void deleteBackup(const QString& absoluteFilePath) override {}

    // Create a full backup
    void consolidateBackup(QuaZip& zip) override {}

private:
    QString _entitiesFilePath;
    QString _entitiesReplacementFilePath;
};

#endif /* hifi_EntitiesBackupHandler_h */
