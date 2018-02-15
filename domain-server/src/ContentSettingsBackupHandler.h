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

    void loadBackup(QuaZip& zip) {};

    void createBackup(QuaZip& zip);

    void recoverBackup(QuaZip& zip);

    void deleteBackup(QuaZip& zip) {};

    void consolidateBackup(QuaZip& zip) {};
private:
    DomainServerSettingsManager& _settingsManager;
};

#endif // hifi_ContentSettingsBackupHandler_h
