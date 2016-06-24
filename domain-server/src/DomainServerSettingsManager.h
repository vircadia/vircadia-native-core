//
//  DomainServerSettingsManager.h
//  domain-server/src
//
//  Created by Stephen Birarda on 2014-06-24.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DomainServerSettingsManager_h
#define hifi_DomainServerSettingsManager_h

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>

#include <HifiConfigVariantMap.h>
#include <HTTPManager.h>

#include <ReceivedMessage.h>
#include "NodePermissions.h"

const QString SETTINGS_PATHS_KEY = "paths";

const QString SETTINGS_PATH = "/settings";
const QString SETTINGS_PATH_JSON = SETTINGS_PATH + ".json";
const QString AGENT_STANDARD_PERMISSIONS_KEYPATH = "security.standard_permissions";
const QString AGENT_PERMISSIONS_KEYPATH = "security.permissions";
const QString GROUP_PERMISSIONS_KEYPATH = "security.group_permissions";
const QString GROUP_FORBIDDENS_KEYPATH = "security.group_forbiddens";

class DomainServerSettingsManager : public QObject {
    Q_OBJECT
public:
    DomainServerSettingsManager();
    bool handlePublicHTTPRequest(HTTPConnection* connection, const QUrl& url);
    bool handleAuthenticatedHTTPRequest(HTTPConnection* connection, const QUrl& url);

    void setupConfigMap(const QStringList& argumentList);
    QVariant valueOrDefaultValueForKeyPath(const QString& keyPath);

    QVariantMap& getUserSettingsMap() { return _configMap.getUserConfig(); }
    QVariantMap& getSettingsMap() { return _configMap.getMergedConfig(); }

    // these give access to anonymous/localhost/logged-in settings from the domain-server settings page
    bool haveStandardPermissionsForName(const QString& name) const { return _standardAgentPermissions.contains(name); }
    NodePermissions getStandardPermissionsForName(const QString& name) const;

    // these give access to permissions for specific user-names from the domain-server settings page
    bool havePermissionsForName(const QString& name) const { return _agentPermissions.contains(name); }
    NodePermissions getPermissionsForName(const QString& name) const;
    QStringList getAllNames() { return _agentPermissions.keys(); }

    // these give access to permissions for specific groups from the domain-server settings page
    bool havePermissionsForGroup(const QString& groupname) const { return _groupPermissions.contains(groupname); }
    NodePermissions getPermissionsForGroup(const QString& groupname) const;
    NodePermissions getPermissionsForGroup(const QUuid& groupID) const;

    // these remove permissions from users in certain groups
    bool haveForbiddensForGroup(const QString& groupname) const { return _groupForbiddens.contains(groupname); }
    NodePermissions getForbiddensForGroup(const QString& groupname) const;
    NodePermissions getForbiddensForGroup(const QUuid& groupID) const;

    QList<QUuid> getGroupIDs();
    QList<QUuid> getBlacklistGroupIDs();

    // these are used to locally cache the result of calling "api/v1/groups/.../is_member/..." on metaverse's api
    void recordGroupMembership(const QString& name, const QUuid groupID, bool isMember);
    bool isGroupMember(const QString& name, const QUuid& groupID);

signals:
    void updateNodePermissions();

public slots:
    void getGroupIDJSONCallback(QNetworkReply& requestReply);
    void getGroupIDErrorCallback(QNetworkReply& requestReply);

private slots:
    void processSettingsRequestPacket(QSharedPointer<ReceivedMessage> message);

private:
    QStringList _argumentList;

    QJsonObject responseObjectForType(const QString& typeValue, bool isAuthenticated = false);
    bool recurseJSONObjectAndOverwriteSettings(const QJsonObject& postedObject);

    void updateSetting(const QString& key, const QJsonValue& newValue, QVariantMap& settingMap,
                       const QJsonObject& settingDescription);
    QJsonObject settingDescriptionFromGroup(const QJsonObject& groupObject, const QString& settingName);
    void sortPermissions();
    void persistToFile();

    double _descriptionVersion;
    QJsonArray _descriptionArray;
    HifiConfigVariantMap _configMap;

    friend class DomainServer;

    // these cause calls to metaverse's group api
    void requestMissingGroupIDs();
    void getGroupID(const QString& groupname);
    NodePermissionsPointer lookupGroupByID(const QUuid& id);

    void packPermissionsForMap(QString mapName, NodePermissionsMap& agentPermissions, QString keyPath);
    void packPermissions();
    void unpackPermissions();

    NodePermissionsMap _standardAgentPermissions; // anonymous, logged-in, localhost
    NodePermissionsMap _agentPermissions; // specific account-names
    NodePermissionsMap _groupPermissions; // permissions granted by membership to specific groups
    NodePermissionsMap _groupForbiddens; // permissions denied due to membership in a specific group
    QHash<QUuid, NodePermissionsPointer> _groupByID; // similar to _groupPermissions but key is group-id rather than name

    // keep track of answers to api queries about which users are in which groups
    QHash<QString, QHash<QUuid, bool>> _groupMembership;
};

#endif // hifi_DomainServerSettingsManager_h
