//
//  AgentPermissions.h
//  libraries/networking/src/
//
//  Created by Seth Alves on 2016-6-1.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AgentPermissions_h
#define hifi_AgentPermissions_h

#include <memory>
#include <QString>
#include <QMap>
#include <QVariant>
#include <QUuid>

class AgentPermissions;
using AgentPermissionsPointer = std::shared_ptr<AgentPermissions>;

class AgentPermissions {
public:
    AgentPermissions() { _id = QUuid::createUuid().toString(); }
    AgentPermissions(const QString& name) { _id = name; }
    AgentPermissions(QMap<QString, QVariant> perms) {
        _id = perms["permissions_id"].toString();
        canConnectToDomain = perms["id_can_connect"].toBool();
        canAdjustLocks = perms["id_can_adjust_locks"].toBool();
        canRezPermanentEntities = perms["id_can_rez"].toBool();
        canRezTemporaryEntities = perms["id_can_rez_tmp"].toBool();
        canWriteToAssetServer = perms["id_can_write_to_asset_server"].toBool();
        canConnectPastMaxCapacity = perms["id_can_connect_past_max_capacity"].toBool();
    }

    QString getID() const { return _id; }

    // the initializations here should match the defaults in describe-settings.json
    bool canConnectToDomain { true };
    bool canAdjustLocks { false };
    bool canRezPermanentEntities { false };
    bool canRezTemporaryEntities { false };
    bool canWriteToAssetServer { false };
    bool canConnectPastMaxCapacity { false };

    void setAll(bool value) {
        canConnectToDomain = value;
        canAdjustLocks = value;
        canRezPermanentEntities = value;
        canRezTemporaryEntities = value;
        canWriteToAssetServer = value;
        canConnectPastMaxCapacity = value;
    }

    QVariant toVariant() {
        QMap<QString, QVariant> values;
        values["permissions_id"] = _id;
        values["id_can_connect"] = canConnectToDomain;
        values["id_can_adjust_locks"] = canAdjustLocks;
        values["id_can_rez"] = canRezPermanentEntities;
        values["id_can_rez_tmp"] = canRezTemporaryEntities;
        values["id_can_write_to_asset_server"] = canWriteToAssetServer;
        values["id_can_connect_past_max_capacity"] = canConnectPastMaxCapacity;
        return QVariant(values);
    }

    AgentPermissions& operator|=(const AgentPermissions& rhs);
    friend QDataStream& operator<<(QDataStream& out, const AgentPermissions& perms);
    friend QDataStream& operator>>(QDataStream& in, AgentPermissions& perms);

protected:
    QString _id;
};

const AgentPermissions DEFAULT_AGENT_PERMISSIONS;

QDebug operator<<(QDebug debug, const AgentPermissions& node);

#endif // hifi_AgentPermissions_h
