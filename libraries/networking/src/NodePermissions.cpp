//
//  NodePermissions.cpp
//  libraries/networking/src/
//
//  Created by Seth Alves on 2016-6-1.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDataStream>
#include <QtCore/QDebug>
#include "NodePermissions.h"

QString NodePermissions::standardNameLocalhost = QString("localhost");
QString NodePermissions::standardNameLoggedIn = QString("logged-in");
QString NodePermissions::standardNameAnonymous = QString("anonymous");
QString NodePermissions::standardNameFriends = QString("friends");

QStringList NodePermissions::standardNames = QList<QString>()
    << NodePermissions::standardNameLocalhost
    << NodePermissions::standardNameLoggedIn
    << NodePermissions::standardNameAnonymous
    << NodePermissions::standardNameFriends;

NodePermissions::NodePermissions(QMap<QString, QVariant> perms) {
    _id = perms["permissions_id"].toString();
    if (perms.contains("group_id")) {
        _groupIDSet = true;
        _groupID = perms["group_id"].toUuid();
    }

    permissions = NodePermissions::Permissions();
    permissions |= perms["id_can_connect"].toBool() ? Permission::canConnectToDomain : Permission::none;
    permissions |= perms["id_can_adjust_locks"].toBool() ? Permission::canAdjustLocks : Permission::none;
    permissions |= perms["id_can_rez"].toBool() ? Permission::canRezPermanentEntities : Permission::none;
    permissions |= perms["id_can_rez_tmp"].toBool() ? Permission::canRezTemporaryEntities : Permission::none;
    permissions |= perms["id_can_write_to_asset_server"].toBool() ? Permission::canWriteToAssetServer : Permission::none;
    permissions |= perms["id_can_connect_past_max_capacity"].toBool() ?
        Permission::canConnectPastMaxCapacity : Permission::none;
}

QVariant NodePermissions::toVariant() {
    QMap<QString, QVariant> values;
    values["permissions_id"] = _id;
    if (_groupIDSet) {
        values["group_id"] = _groupID;
    }
    values["id_can_connect"] = can(Permission::canConnectToDomain);
    values["id_can_adjust_locks"] = can(Permission::canAdjustLocks);
    values["id_can_rez"] = can(Permission::canRezPermanentEntities);
    values["id_can_rez_tmp"] = can(Permission::canRezTemporaryEntities);
    values["id_can_write_to_asset_server"] = can(Permission::canWriteToAssetServer);
    values["id_can_connect_past_max_capacity"] = can(Permission::canConnectPastMaxCapacity);
    return QVariant(values);
}

void NodePermissions::setAll(bool value) {
    permissions = NodePermissions::Permissions();
    if (value) {
        permissions = ~permissions;
    }
}

NodePermissions& NodePermissions::operator|=(const NodePermissions& rhs) {
    permissions |= rhs.permissions;
    return *this;
}
NodePermissions& NodePermissions::operator|=(const NodePermissionsPointer& rhs) {
    if (rhs) {
        *this |= *rhs.get();
    }
    return *this;
}
NodePermissionsPointer& operator|=(NodePermissionsPointer& lhs, const NodePermissionsPointer& rhs) {
    if (lhs && rhs) {
        *lhs.get() |= rhs;
    }
    return lhs;
}
NodePermissionsPointer& operator|=(NodePermissionsPointer& lhs, NodePermissions::Permission rhs) {
    if (lhs) {
        lhs.get()->permissions |= rhs;
    }
    return lhs;
}

NodePermissions& NodePermissions::operator&=(const NodePermissions& rhs) {
    permissions &= rhs.permissions;
    return *this;
}
NodePermissions& NodePermissions::operator&=(const NodePermissionsPointer& rhs) {
    if (rhs) {
        *this &= *rhs.get();
    }
    return *this;
}
NodePermissionsPointer& operator&=(NodePermissionsPointer& lhs, const NodePermissionsPointer& rhs) {
    if (lhs && rhs) {
        *lhs.get() &= rhs;
    }
    return lhs;
}
NodePermissionsPointer& operator&=(NodePermissionsPointer& lhs, NodePermissions::Permission rhs) {
    if (lhs) {
        lhs.get()->permissions &= rhs;
    }
    return lhs;
}

NodePermissions NodePermissions::operator~() {
    NodePermissions result = *this;
    result.permissions = ~permissions;
    return result;
}

NodePermissionsPointer operator~(NodePermissionsPointer& lhs) {
    if (lhs) {
        NodePermissionsPointer result { new NodePermissions };
        (*result.get()) = ~(*lhs.get());
        return result;
    }
    return lhs;
}

QDataStream& operator<<(QDataStream& out, const NodePermissions& perms) {
    out << (uint)perms.permissions;
    return out;
}

QDataStream& operator>>(QDataStream& in, NodePermissions& perms) {
    uint permissionsInt;
    in >> permissionsInt;
    perms.permissions = (NodePermissions::Permissions)permissionsInt;
    return in;
}

QDebug operator<<(QDebug debug, const NodePermissions& perms) {
    debug.nospace() << "[permissions: " << perms.getID() << " --";
    if (perms.can(NodePermissions::Permission::canConnectToDomain)) {
        debug << " connect";
    }
    if (perms.can(NodePermissions::Permission::canAdjustLocks)) {
        debug << " locks";
    }
    if (perms.can(NodePermissions::Permission::canRezPermanentEntities)) {
        debug << " rez";
    }
    if (perms.can(NodePermissions::Permission::canRezTemporaryEntities)) {
        debug << " rez-tmp";
    }
    if (perms.can(NodePermissions::Permission::canWriteToAssetServer)) {
        debug << " asset-server";
    }
    if (perms.can(NodePermissions::Permission::canConnectPastMaxCapacity)) {
        debug << " ignore-max-cap";
    }
    debug.nospace() << "]";
    return debug.nospace();
}
QDebug operator<<(QDebug debug, const NodePermissionsPointer& perms) {
    if (perms) {
        return operator<<(debug, *perms.get());
    }
    debug.nospace() << "[permissions: null]";
    return debug.nospace();
}
