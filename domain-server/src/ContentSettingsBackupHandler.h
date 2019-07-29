//
//  ContentSettingsBackupHandler.h
//  domain-server/src
//
//  Created by Stephen Birarda on 2/15/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ContentSettingsBackupHandler_h
#define hifi_ContentSettingsBackupHandler_h

#include "BackupHandler.h"
#include "DomainServerSettingsManager.h"

class ContentSettingsBackupHandler : public BackupHandlerInterface {
public:
    ContentSettingsBackupHandler(DomainServerSettingsManager& domainServerSettingsManager);

    std::pair<bool, float> isAvailable(const QString& backupName) override { return { true, 1.0f }; }
    std::pair<bool, float> getRecoveryStatus() override { return { false, 1.0f }; }

    void loadBackup(const QString& backupName, QuaZip& zip) override {}

    void loadingComplete() override {}

    void createBackup(const QString& backupName, QuaZip& zip) override;

    std::pair<bool, QString> recoverBackup(const QString& backupName, QuaZip& zip, const QString& username, const QString& sourceFilename) override;

    void deleteBackup(const QString& backupName) override {}

    void consolidateBackup(const QString& backupName, QuaZip& zip) override {}

    bool isCorruptedBackup(const QString& backupName) override { return false; }

private:
    DomainServerSettingsManager& _settingsManager;
};

#endif // hifi_ContentSettingsBackupHandler_h
