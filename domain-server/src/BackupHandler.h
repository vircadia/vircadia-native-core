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

class QuaZip;

class BackupHandlerInterface {
public:
    virtual ~BackupHandlerInterface() = default;

    virtual void loadBackup(QuaZip& zip) = 0;
    virtual void createBackup(QuaZip& zip) = 0;
    virtual void recoverBackup(QuaZip& zip) = 0;
    virtual void deleteBackup(QuaZip& zip) = 0;
    virtual void consolidateBackup(QuaZip& zip) = 0;
};

using BackupHandlerPointer = std::unique_ptr<BackupHandlerInterface>;

#endif /* hifi_BackupHandler_h */
