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

    std::pair<bool, float> isAvailable(QString filePath) override { return { true, 1.0f }; }
    std::pair<bool, float> getRecoveryStatus() override { return { false, 1.0f }; }

    void loadBackup(QuaZip& zip) override {}

    void createBackup(QuaZip& zip) override;

    void recoverBackup(QuaZip& zip) override;

    void deleteBackup(QuaZip& zip) override {}

    void consolidateBackup(QuaZip& zip) override {}
private:
    DomainServerSettingsManager& _settingsManager;
};

#endif // hifi_ContentSettingsBackupHandler_h
