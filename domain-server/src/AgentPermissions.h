//
//  AgentPermissions.h
//  domain-server/src
//
//  Created by Seth Alves on 2016-6-1.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AgentPermissions_h
#define hifi_AgentPermissions_h

class AgentPermissions {
public:
    AgentPermissions(QMap<QString, QVariant> perms) {
        _id = perms["permissions_id"].toString();
        canConnectToDomain = perms["id_can_connect"].toBool();
        canAdjustLocks = perms["id_can_adjust_locks"].toBool();
        canRezPermanentEntities = perms["id_can_rez"].toBool();
        canRezTemporaryEntities = perms["id_can_rez_tmp"].toBool();
        canWriteToAssetServer = perms["id_can_write_to_asset_server"].toBool();
    };

    QString getID() { return _id; }

    bool canConnectToDomain { false };
    bool canAdjustLocks { false };
    bool canRezPermanentEntities { false };
    bool canRezTemporaryEntities { false };
    bool canWriteToAssetServer { false };

protected:
    QString _id;
};

using AgentPermissionsPointer = std::shared_ptr<AgentPermissions>;

#endif // hifi_AgentPermissions_h
