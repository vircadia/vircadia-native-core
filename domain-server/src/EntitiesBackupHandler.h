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

#include <QString>

#include "BackupHandler.h"

class EntitiesBackupHandler : public BackupHandlerInterface {
public:
    EntitiesBackupHandler(QString entitiesFilePath, QString entitiesReplacementFilePath);

    void loadBackup(QuaZip& zip) {}

    // Create a skeleton backup
    void createBackup(QuaZip& zip);

    // Recover from a full backup
    void recoverBackup(QuaZip& zip);

    // Delete a skeleton backup
    void deleteBackup(QuaZip& zip) {}

    // Create a full backup
    void consolidateBackup(QuaZip& zip) {}

private:
    QString _entitiesFilePath;
    QString _entitiesReplacementFilePath;
};

#endif /* hifi_EntitiesBackupHandler_h */
