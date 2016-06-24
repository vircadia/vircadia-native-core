//
//  NodePermissions.h
//  libraries/networking/src/
//
//  Created by Seth Alves on 2016-6-1.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NodePermissions_h
#define hifi_NodePermissions_h

#include <memory>
#include <QString>
#include <QMap>
#include <QVariant>
#include <QUuid>

class NodePermissions;
using NodePermissionsPointer = std::shared_ptr<NodePermissions>;

class NodePermissions {
public:
    NodePermissions() { _id = QUuid::createUuid().toString(); }
    NodePermissions(const QString& name) { _id = name.toLower(); }
    NodePermissions(QMap<QString, QVariant> perms);

    QString getID() const { return _id; }

    // the _id member isn't authenticated and _username is.
    void setUserName(QString userName) { _userName = userName.toLower(); }
    QString getUserName() { return _userName; }

    void setGroupID(QUuid groupID) { _groupID = groupID; if (!groupID.isNull()) { _groupIDSet = true; } }
    QUuid getGroupID() { return _groupID; }
    bool isGroup() { return _groupIDSet; }

    bool isAssignment { false };

    // these 3 names have special meaning.
    static QString standardNameLocalhost;
    static QString standardNameLoggedIn;
    static QString standardNameAnonymous;
    static QString standardNameFriends;
    static QStringList standardNames;

    // the initializations here should match the defaults in describe-settings.json
    bool canConnectToDomain { true };
    bool canAdjustLocks { false };
    bool canRezPermanentEntities { false };
    bool canRezTemporaryEntities { false };
    bool canWriteToAssetServer { false };
    bool canConnectPastMaxCapacity { false };

    QVariant toVariant();

    void setAll(bool value);

    NodePermissions& operator|=(const NodePermissions& rhs);
    NodePermissions& operator|=(const NodePermissionsPointer& rhs);
    friend QDataStream& operator<<(QDataStream& out, const NodePermissions& perms);
    friend QDataStream& operator>>(QDataStream& in, NodePermissions& perms);

protected:
    QString _id;
    QString _userName;

    bool _groupIDSet { false };
    QUuid _groupID;
};


// wrap QHash in a class that forces all keys to be lowercase
class NodePermissionsMap {
public:
    NodePermissionsMap() { }
    NodePermissionsPointer& operator[](const QString& key) { return _data[key.toLower()]; }
    NodePermissionsPointer operator[](const QString& key) const { return _data.value(key.toLower()); }
    bool contains(const QString& key) const { return _data.contains(key.toLower()); }
    QList<QString> keys() const { return _data.keys(); }
    QHash<QString, NodePermissionsPointer> get() { return _data; }
    void clear() { _data.clear(); }

private:
    QHash<QString, NodePermissionsPointer> _data;
};


const NodePermissions DEFAULT_AGENT_PERMISSIONS;

QDebug operator<<(QDebug debug, const NodePermissions& perms);
QDebug operator<<(QDebug debug, const NodePermissionsPointer& perms);
NodePermissionsPointer& operator|=(NodePermissionsPointer& lhs, const NodePermissionsPointer& rhs);

#endif // hifi_NodePermissions_h
