//
//  DomainServerSettingsManager.h
//  domain-server/src
//
//  Created by Stephen Birarda on 2014-06-24.
//  Copyright 2014 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DomainServerSettingsManager_h
#define hifi_DomainServerSettingsManager_h

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QSharedPointer>

#include <HifiConfigVariantMap.h>
#include <HTTPManager.h>
#include <Node.h>
#include <ReceivedMessage.h>

#include "DomainGatekeeper.h"
#include "NodePermissions.h"

const QString SETTINGS_PATHS_KEY = "paths";

const QString AGENT_STANDARD_PERMISSIONS_KEYPATH = "security.standard_permissions";
const QString AGENT_PERMISSIONS_KEYPATH = "security.permissions";
const QString IP_PERMISSIONS_KEYPATH = "security.ip_permissions";
const QString MAC_PERMISSIONS_KEYPATH = "security.mac_permissions";
const QString MACHINE_FINGERPRINT_PERMISSIONS_KEYPATH = "security.machine_fingerprint_permissions";
const QString GROUP_PERMISSIONS_KEYPATH = "security.group_permissions";
const QString GROUP_FORBIDDENS_KEYPATH = "security.group_forbiddens";
const QString AUTOMATIC_CONTENT_ARCHIVES_GROUP = "automatic_content_archives";
const QString CONTENT_SETTINGS_INSTALLED_CONTENT_FILENAME = "installed_content.filename";
const QString CONTENT_SETTINGS_INSTALLED_CONTENT_NAME = "installed_content.name";
const QString CONTENT_SETTINGS_INSTALLED_CONTENT_CREATION_TIME = "installed_content.creation_time";
const QString CONTENT_SETTINGS_INSTALLED_CONTENT_INSTALL_TIME = "installed_content.install_time";
const QString CONTENT_SETTINGS_INSTALLED_CONTENT_INSTALLED_BY = "installed_content.installed_by";

using GroupByUUIDKey = QPair<QUuid, QUuid>; // groupID, rankID

enum SettingsType {
    DomainSettings,
    ContentSettings
};

class DomainServerSettingsManager : public QObject {
    Q_OBJECT
public:
    DomainServerSettingsManager();
    bool handleAuthenticatedHTTPRequest(HTTPConnection* connection, const QUrl& url);

    void setupConfigMap(const QString& userConfigFilename);

    // each of the three methods in this group takes a read lock of _settingsLock
    // and cannot be called when the a write lock is held by the same thread
    QVariant valueOrDefaultValueForKeyPath(const QString& keyPath);
    QVariant valueForKeyPath(const QString& keyPath);
    bool containsKeyPath(const QString& keyPath) { return valueForKeyPath(keyPath).isValid(); }

    // these give access to anonymous/localhost/logged-in settings from the domain-server settings page
    bool haveStandardPermissionsForName(const QString& name) const { return _standardAgentPermissions.contains(name, 0); }
    NodePermissions getStandardPermissionsForName(const NodePermissionsKey& name) const;

    // these give access to permissions for specific user-names from the domain-server settings page
    bool havePermissionsForName(const QString& name) const { return _agentPermissions.contains(name, 0); }
    NodePermissions getPermissionsForName(const QString& name) const;
    NodePermissions getPermissionsForName(const NodePermissionsKey& key) const { return getPermissionsForName(key.first); }
    QStringList getAllNames() const;

    // these give access to permissions for specific IPs from the domain-server settings page
    bool hasPermissionsForIP(const QHostAddress& address) const { return _ipPermissions.contains(address.toString(), 0); }
    NodePermissions getPermissionsForIP(const QHostAddress& address) const;

    // these give access to permissions for specific MACs from the domain-server settings page
    bool hasPermissionsForMAC(const QString& macAddress) const { return _macPermissions.contains(macAddress, 0); }
    NodePermissions getPermissionsForMAC(const QString& macAddress) const;

    // these give access to permissions for specific machine fingerprints from the domain-server settings page
    bool hasPermissionsForMachineFingerprint(const QUuid& machineFingerprint) { return _machineFingerprintPermissions.contains(machineFingerprint.toString(), 0); }
    NodePermissions getPermissionsForMachineFingerprint(const QUuid& machineFingerprint) const;

    // these give access to permissions for specific groups from the domain-server settings page
    bool havePermissionsForGroup(const QString& groupName, QUuid rankID) const {
        return _groupPermissions.contains(groupName, rankID);
    }
    NodePermissions getPermissionsForGroup(const QString& groupName, QUuid rankID) const;
    NodePermissions getPermissionsForGroup(const QUuid& groupID, QUuid rankID) const;

    // these remove permissions from users in certain groups
    bool haveForbiddensForGroup(const QString& groupName, QUuid rankID) const {
        return _groupForbiddens.contains(groupName, rankID);
    }
    NodePermissions getForbiddensForGroup(const QString& groupName, QUuid rankID) const;
    NodePermissions getForbiddensForGroup(const QUuid& groupID, QUuid rankID) const;

    QStringList getAllKnownGroupNames();
    bool setGroupID(const QString& groupName, const QUuid& groupID);
    GroupRank getGroupRank(QUuid groupID, QUuid rankID) { return _groupRanks[groupID][rankID]; }

    QList<QUuid> getGroupIDs();
    QList<QUuid> getBlacklistGroupIDs();

    QStringList getDomainServerGroupNames();
    QStringList getDomainServerBlacklistGroupNames();

    // these are used to locally cache the result of calling "/api/v1/groups/.../is_member/..." on metaverse's api
    void clearGroupMemberships(const QString& name) { _groupMembership[name.toLower()].clear(); }
    void recordGroupMembership(const QString& name, const QUuid groupID, QUuid rankID);
    QUuid isGroupMember(const QString& name, const QUuid& groupID); // returns rank or -1 if not a member

    // calls http api to refresh group information
    void apiRefreshGroupInformation();

    void debugDumpGroupsState();

    enum SettingsRequestAuthentication { NotAuthenticated, Authenticated };
    enum DomainSettingsInclusion { NoDomainSettings, IncludeDomainSettings };
    enum ContentSettingsInclusion { NoContentSettings, IncludeContentSettings };
    enum DefaultSettingsInclusion { NoDefaultSettings, IncludeDefaultSettings };
    enum SettingsBackupFlag { NotForBackup, ForBackup };

    /// thread safe method to retrieve a JSON representation of settings
    QJsonObject settingsResponseObjectForType(const QString& typeValue,
                                              SettingsRequestAuthentication authentication = NotAuthenticated,
                                              DomainSettingsInclusion domainSettingsInclusion = IncludeDomainSettings,
                                              ContentSettingsInclusion contentSettingsInclusion = IncludeContentSettings,
                                              DefaultSettingsInclusion defaultSettingsInclusion = IncludeDefaultSettings,
                                              SettingsBackupFlag settingsBackupFlag = NotForBackup);
    /// thread safe method to restore settings from a JSON object
    Q_INVOKABLE bool restoreSettingsFromObject(QJsonObject settingsToRestore, SettingsType settingsType);

    bool recurseJSONObjectAndOverwriteSettings(const QJsonObject& postedObject, SettingsType settingsType);

signals:
    void updateNodePermissions();
    void settingsUpdated();

public slots:
    void apiGetGroupIDJSONCallback(QNetworkReply* requestReply);
    void apiGetGroupIDErrorCallback(QNetworkReply* requestReply);
    void apiGetGroupRanksJSONCallback(QNetworkReply* requestReply);
    void apiGetGroupRanksErrorCallback(QNetworkReply* requestReply);

private slots:
    void processSettingsRequestPacket(QSharedPointer<ReceivedMessage> message);
    void processNodeKickRequestPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);
    void processUsernameFromIDRequestPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);

private:
    QJsonArray filteredDescriptionArray(bool isContentSettings);
    void updateSetting(const QString& key, const QJsonValue& newValue, QVariantMap& settingMap,
                       const QJsonObject& settingDescription);
    QJsonObject settingDescriptionFromGroup(const QJsonObject& groupObject, const QString& settingName);
    void sortPermissions();

    // you cannot be holding the _settingsLock when persisting to file from the same thread
    // since it may take either a read lock or write lock and recursive locking doesn't allow a change in type
    void persistToFile();

    void splitSettingsDescription();

    double _descriptionVersion;

    QJsonArray _descriptionArray;
    QJsonArray _domainSettingsDescription;
    QJsonArray _contentSettingsDescription;
    QJsonObject _settingsMenuGroups;

    // any method that calls valueForKeyPath on this _configMap must get a write lock it keeps until it
    // is done with the returned QVariant*
    HifiConfigVariantMap _configMap;

    // these cause calls to metaverse's group api
    void apiGetGroupID(const QString& groupName);
    void apiGetGroupRanks(const QUuid& groupID);

    void initializeGroupPermissions(NodePermissionsMap& permissionsRows, QString groupName, NodePermissionsPointer perms);
    void packPermissionsForMap(QString mapName, NodePermissionsMap& permissionsRows, QString keyPath);
    void packPermissions();
    void unpackPermissions();
    bool unpackPermissionsForKeypath(const QString& keyPath, NodePermissionsMap* destinationMapPointer,
                                     std::function<void(NodePermissionsPointer)> customUnpacker = {});
    bool ensurePermissionsForGroupRanks();

    NodePermissionsMap _standardAgentPermissions; // anonymous, logged-in, localhost, friend-of-domain-owner
    NodePermissionsMap _agentPermissions; // specific account-names

    NodePermissionsMap _ipPermissions; // permissions granted by node IP address
    NodePermissionsMap _macPermissions; // permissions granted by node MAC address
    NodePermissionsMap _machineFingerprintPermissions; // permissions granted by Machine Fingerprint

    NodePermissionsMap _groupPermissions; // permissions granted by membership to specific groups
    NodePermissionsMap _groupForbiddens; // permissions denied due to membership in a specific group
    // these are like _groupPermissions and _groupForbiddens but with uuids rather than group-names in the keys
    QHash<GroupByUUIDKey, NodePermissionsPointer> _groupPermissionsByUUID;
    QHash<GroupByUUIDKey, NodePermissionsPointer> _groupForbiddensByUUID;

    QHash<QString, QUuid> _groupIDs; // keep track of group-name to group-id mappings
    QHash<QUuid, QString> _groupNames; // keep track of group-id to group-name mappings

    // remember the responses to api/v1/groups/%1/ranks
    QHash<QUuid, QHash<QUuid, GroupRank>> _groupRanks; // QHash<group-id, QHash<rankID, rank>>

    // keep track of answers to api queries about which users are in which groups
    QHash<QString, QHash<QUuid, QUuid>> _groupMembership; // QHash<user-name, QHash<group-id, rank-id>>

    /// guard read/write access from multiple threads to settings 
    QReadWriteLock _settingsLock { QReadWriteLock::Recursive };

    friend class DomainServer;
};

#endif // hifi_DomainServerSettingsManager_h
