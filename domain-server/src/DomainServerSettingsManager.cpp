//
//  DomainServerSettingsManager.cpp
//  domain-server/src
//
//  Created by Stephen Birarda on 2014-06-24.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <algorithm>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <AccountManager.h>
#include <QTimeZone>

#include <Assignment.h>
#include <HifiConfigVariantMap.h>
#include <HTTPConnection.h>
#include <NLPacketList.h>
#include <NumericalConstants.h>

#include "DomainServerNodeData.h"

#include "DomainServerSettingsManager.h"

const QString SETTINGS_DESCRIPTION_RELATIVE_PATH = "/resources/describe-settings.json";

const QString DESCRIPTION_SETTINGS_KEY = "settings";
const QString SETTING_DEFAULT_KEY = "default";
const QString DESCRIPTION_NAME_KEY = "name";
const QString SETTING_DESCRIPTION_TYPE_KEY = "type";
const QString DESCRIPTION_COLUMNS_KEY = "columns";

const QString SETTINGS_VIEWPOINT_KEY = "viewpoint";

DomainServerSettingsManager::DomainServerSettingsManager() :
    _descriptionArray(),
    _configMap()
{
    // load the description object from the settings description
    QFile descriptionFile(QCoreApplication::applicationDirPath() + SETTINGS_DESCRIPTION_RELATIVE_PATH);
    descriptionFile.open(QIODevice::ReadOnly);

    QJsonParseError parseError;
    QJsonDocument descriptionDocument = QJsonDocument::fromJson(descriptionFile.readAll(), &parseError);

    if (descriptionDocument.isObject()) {
        QJsonObject descriptionObject = descriptionDocument.object();

        const QString DESCRIPTION_VERSION_KEY = "version";

        if (descriptionObject.contains(DESCRIPTION_VERSION_KEY)) {
            // read the version from the settings description
            _descriptionVersion = descriptionObject[DESCRIPTION_VERSION_KEY].toDouble();

            if (descriptionObject.contains(DESCRIPTION_SETTINGS_KEY)) {
                _descriptionArray = descriptionDocument.object()[DESCRIPTION_SETTINGS_KEY].toArray();
                return;
            }
        }
    }

    static const QString MISSING_SETTINGS_DESC_MSG =
        QString("Did not find settings description in JSON at %1 - Unable to continue. domain-server will quit.\n%2 at %3")
        .arg(SETTINGS_DESCRIPTION_RELATIVE_PATH).arg(parseError.errorString()).arg(parseError.offset);
    static const int MISSING_SETTINGS_DESC_ERROR_CODE = 6;

    QMetaObject::invokeMethod(QCoreApplication::instance(), "queuedQuit", Qt::QueuedConnection,
                              Q_ARG(QString, MISSING_SETTINGS_DESC_MSG),
                              Q_ARG(int, MISSING_SETTINGS_DESC_ERROR_CODE));
}

void DomainServerSettingsManager::processSettingsRequestPacket(QSharedPointer<ReceivedMessage> message) {
    Assignment::Type type;
    message->readPrimitive(&type);

    QJsonObject responseObject = responseObjectForType(QString::number(type));
    auto json = QJsonDocument(responseObject).toJson();

    auto packetList = NLPacketList::create(PacketType::DomainSettings, QByteArray(), true, true);

    packetList->write(json);

    auto nodeList = DependencyManager::get<LimitedNodeList>();
    nodeList->sendPacketList(std::move(packetList), message->getSenderSockAddr());
}

void DomainServerSettingsManager::setupConfigMap(const QStringList& argumentList) {
    _argumentList = argumentList;

    // after 1.7 we no longer use the master or merged configs - this is kept in place for migration
    _configMap.loadMasterAndUserConfig(_argumentList);

    // What settings version were we before and what are we using now?
    // Do we need to do any re-mapping?
    QSettings appSettings;
    const QString JSON_SETTINGS_VERSION_KEY = "json-settings/version";
    double oldVersion = appSettings.value(JSON_SETTINGS_VERSION_KEY, 0.0).toDouble();

    if (oldVersion != _descriptionVersion) {
        const QString ALLOWED_USERS_SETTINGS_KEYPATH = "security.allowed_users";
        const QString RESTRICTED_ACCESS_SETTINGS_KEYPATH = "security.restricted_access";
        const QString ALLOWED_EDITORS_SETTINGS_KEYPATH = "security.allowed_editors";
        const QString EDITORS_ARE_REZZERS_KEYPATH = "security.editors_are_rezzers";

        qDebug() << "Previous domain-server settings version was"
            << QString::number(oldVersion, 'g', 8) << "and the new version is"
            << QString::number(_descriptionVersion, 'g', 8) << "- checking if any re-mapping is required";

        // we have a version mismatch - for now handle custom behaviour here since there are not many remappings
        if (oldVersion < 1.0) {
            // This was prior to the introduction of security.restricted_access
            // If the user has a list of allowed users then set their value for security.restricted_access to true

            QVariant* allowedUsers = _configMap.valueForKeyPath(ALLOWED_USERS_SETTINGS_KEYPATH);

            if (allowedUsers
                && allowedUsers->canConvert(QMetaType::QVariantList)
                && reinterpret_cast<QVariantList*>(allowedUsers)->size() > 0) {

                qDebug() << "Forcing security.restricted_access to TRUE since there was an"
                    << "existing list of allowed users.";

                // In the pre-toggle system the user had a list of allowed users, so
                // we need to set security.restricted_access to true
                QVariant* restrictedAccess = _configMap.valueForKeyPath(RESTRICTED_ACCESS_SETTINGS_KEYPATH, true);

                *restrictedAccess = QVariant(true);

                // write the new settings to the json file
                persistToFile();

                // reload the master and user config so that the merged config is right
                _configMap.loadMasterAndUserConfig(_argumentList);
            }
        }

        if (oldVersion < 1.1) {
            static const QString ENTITY_SERVER_SETTINGS_KEY = "entity_server_settings";
            static const QString ENTITY_FILE_NAME_KEY = "persistFilename";
            static const QString ENTITY_FILE_PATH_KEYPATH = ENTITY_SERVER_SETTINGS_KEY + ".persistFilePath";

            // this was prior to change of poorly named entitiesFileName to entitiesFilePath
            QVariant* persistFileNameVariant = _configMap.valueForKeyPath(ENTITY_SERVER_SETTINGS_KEY + "." + ENTITY_FILE_NAME_KEY);
            if (persistFileNameVariant && persistFileNameVariant->canConvert(QMetaType::QString)) {
                QString persistFileName = persistFileNameVariant->toString();

                qDebug() << "Migrating persistFilename to persistFilePath for entity-server settings";

                // grab the persistFilePath option, create it if it doesn't exist
                QVariant* persistFilePath = _configMap.valueForKeyPath(ENTITY_FILE_PATH_KEYPATH, true);

                // write the migrated value
                *persistFilePath = persistFileName;

                // remove the old setting
                QVariant* entityServerVariant = _configMap.valueForKeyPath(ENTITY_SERVER_SETTINGS_KEY);
                if (entityServerVariant && entityServerVariant->canConvert(QMetaType::QVariantMap)) {
                    QVariantMap entityServerMap = entityServerVariant->toMap();
                    entityServerMap.remove(ENTITY_FILE_NAME_KEY);

                    *entityServerVariant = entityServerMap;
                }

                // write the new settings to the json file
                persistToFile();

                // reload the master and user config so that the merged config is right
                _configMap.loadMasterAndUserConfig(_argumentList);
            }

        }

        if (oldVersion < 1.2) {
            // This was prior to the base64 encoding of password for HTTP Basic Authentication.
            // If we have a password in the previous settings file, make it base 64
            static const QString BASIC_AUTH_PASSWORD_KEY_PATH { "security.http_password" };

            QVariant* passwordVariant = _configMap.valueForKeyPath(BASIC_AUTH_PASSWORD_KEY_PATH);

            if (passwordVariant && passwordVariant->canConvert(QMetaType::QString)) {
                QString plaintextPassword = passwordVariant->toString();

                qDebug() << "Migrating plaintext password to SHA256 hash in domain-server settings.";

                *passwordVariant = QCryptographicHash::hash(plaintextPassword.toUtf8(), QCryptographicHash::Sha256).toHex();

                // write the new settings to file
                persistToFile();

                // reload the master and user config so the merged config is correct
                _configMap.loadMasterAndUserConfig(_argumentList);
            }
        }

        if (oldVersion < 1.4) {
            // This was prior to the permissions-grid in the domain-server settings page
            bool isRestrictedAccess = valueOrDefaultValueForKeyPath(RESTRICTED_ACCESS_SETTINGS_KEYPATH).toBool();
            QStringList allowedUsers = valueOrDefaultValueForKeyPath(ALLOWED_USERS_SETTINGS_KEYPATH).toStringList();
            QStringList allowedEditors = valueOrDefaultValueForKeyPath(ALLOWED_EDITORS_SETTINGS_KEYPATH).toStringList();
            bool onlyEditorsAreRezzers = valueOrDefaultValueForKeyPath(EDITORS_ARE_REZZERS_KEYPATH).toBool();

            _standardAgentPermissions[NodePermissions::standardNameLocalhost].reset(
                new NodePermissions(NodePermissions::standardNameLocalhost));
            _standardAgentPermissions[NodePermissions::standardNameLocalhost]->setAll(true);
            _standardAgentPermissions[NodePermissions::standardNameAnonymous].reset(
                new NodePermissions(NodePermissions::standardNameAnonymous));
            _standardAgentPermissions[NodePermissions::standardNameLoggedIn].reset(
                new NodePermissions(NodePermissions::standardNameLoggedIn));
            _standardAgentPermissions[NodePermissions::standardNameFriends].reset(
                new NodePermissions(NodePermissions::standardNameFriends));

            if (isRestrictedAccess) {
                // only users in allow-users list can connect
                _standardAgentPermissions[NodePermissions::standardNameAnonymous]->clear(
                    NodePermissions::Permission::canConnectToDomain);
                _standardAgentPermissions[NodePermissions::standardNameLoggedIn]->clear(
                    NodePermissions::Permission::canConnectToDomain);
            } // else anonymous and logged-in retain default of canConnectToDomain = true

            foreach (QString allowedUser, allowedUsers) {
                // even if isRestrictedAccess is false, we have to add explicit rows for these users.
                _agentPermissions[NodePermissionsKey(allowedUser, 0)].reset(new NodePermissions(allowedUser));
                _agentPermissions[NodePermissionsKey(allowedUser, 0)]->set(NodePermissions::Permission::canConnectToDomain);
            }

            foreach (QString allowedEditor, allowedEditors) {
                NodePermissionsKey editorKey(allowedEditor, 0);
                if (!_agentPermissions.contains(editorKey)) {
                    _agentPermissions[editorKey].reset(new NodePermissions(allowedEditor));
                    if (isRestrictedAccess) {
                        // they can change locks, but can't connect.
                        _agentPermissions[editorKey]->clear(NodePermissions::Permission::canConnectToDomain);
                    }
                }
                _agentPermissions[editorKey]->set(NodePermissions::Permission::canAdjustLocks);
            }

            QList<QHash<NodePermissionsKey, NodePermissionsPointer>> permissionsSets;
            permissionsSets << _standardAgentPermissions.get() << _agentPermissions.get();
            foreach (auto permissionsSet, permissionsSets) {
                foreach (NodePermissionsKey userKey, permissionsSet.keys()) {
                    if (onlyEditorsAreRezzers) {
                        if (permissionsSet[userKey]->can(NodePermissions::Permission::canAdjustLocks)) {
                            permissionsSet[userKey]->set(NodePermissions::Permission::canRezPermanentEntities);
                            permissionsSet[userKey]->set(NodePermissions::Permission::canRezTemporaryEntities);
                        } else {
                            permissionsSet[userKey]->clear(NodePermissions::Permission::canRezPermanentEntities);
                            permissionsSet[userKey]->clear(NodePermissions::Permission::canRezTemporaryEntities);
                        }
                    } else {
                        permissionsSet[userKey]->set(NodePermissions::Permission::canRezPermanentEntities);
                        permissionsSet[userKey]->set(NodePermissions::Permission::canRezTemporaryEntities);
                    }
                }
            }

            packPermissions();
            _standardAgentPermissions.clear();
            _agentPermissions.clear();
        }

        if (oldVersion < 1.5) {
            // This was prior to operating hours, so add default hours
            validateDescriptorsMap();
        }

        if (oldVersion < 1.6) {
            unpackPermissions();

            // This was prior to addition of kick permissions, add that to localhost permissions by default
            _standardAgentPermissions[NodePermissions::standardNameLocalhost]->set(NodePermissions::Permission::canKick);

            packPermissions();
        }

        if (oldVersion < 1.7) {
            // This was prior to the removal of the master config file
            // So we write the merged config to the user config file, and stop reading from the user config file

            qDebug() << "Migrating merged config to user config file. The master config file is deprecated.";

            // replace the user config by the merged config
            _configMap.getConfig() = _configMap.getMergedConfig();

            // persist the new config so the user config file has the correctly merged config
            persistToFile();
        }
    }

    unpackPermissions();

    // write the current description version to our settings
    appSettings.setValue(JSON_SETTINGS_VERSION_KEY, _descriptionVersion);
}

QVariantMap& DomainServerSettingsManager::getDescriptorsMap() {
    validateDescriptorsMap();

    static const QString DESCRIPTORS{ "descriptors" };
    return *static_cast<QVariantMap*>(getSettingsMap()[DESCRIPTORS].data());
}

void DomainServerSettingsManager::validateDescriptorsMap() {
    static const QString WEEKDAY_HOURS{ "descriptors.weekday_hours" };
    static const QString WEEKEND_HOURS{ "descriptors.weekend_hours" };
    static const QString UTC_OFFSET{ "descriptors.utc_offset" };

    QVariant* weekdayHours = _configMap.valueForKeyPath(WEEKDAY_HOURS, true);
    QVariant* weekendHours = _configMap.valueForKeyPath(WEEKEND_HOURS, true);
    QVariant* utcOffset = _configMap.valueForKeyPath(UTC_OFFSET, true);

    static const QString OPEN{ "open" };
    static const QString CLOSE{ "close" };
    static const QString DEFAULT_OPEN{ "00:00" };
    static const QString DEFAULT_CLOSE{ "23:59" };
    bool wasMalformed = false;
    if (weekdayHours->isNull()) {
        *weekdayHours = QVariantList{ QVariantMap{ { OPEN, QVariant(DEFAULT_OPEN) }, { CLOSE, QVariant(DEFAULT_CLOSE) } } };
        wasMalformed = true;
    }
    if (weekendHours->isNull()) {
        *weekendHours = QVariantList{ QVariantMap{ { OPEN, QVariant(DEFAULT_OPEN) }, { CLOSE, QVariant(DEFAULT_CLOSE) } } };
        wasMalformed = true;
    }
    if (utcOffset->isNull()) {
        *utcOffset = QVariant(QTimeZone::systemTimeZone().offsetFromUtc(QDateTime::currentDateTime()) / (float)SECS_PER_HOUR);
        wasMalformed = true;
    }

    if (wasMalformed) {
        // write the new settings to file
        persistToFile();
    }
}


void DomainServerSettingsManager::initializeGroupPermissions(NodePermissionsMap& permissionsRows,
                                                             QString groupName, NodePermissionsPointer perms) {
    // this is called when someone has used the domain-settings webpage to add a group.  They type the group's name
    // and give it some permissions.  The domain-server asks api for the group's ranks and populates the map
    // with them.  Here, that initial user-entered row is removed and it's permissions are copied to all the ranks
    // except owner.

    QString groupNameLower = groupName.toLower();

    foreach (NodePermissionsKey nameKey, permissionsRows.keys()) {
        if (nameKey.first.toLower() != groupNameLower) {
            continue;
        }
        QUuid groupID = _groupIDs[groupNameLower.toLower()];
        QUuid rankID = nameKey.second;
        GroupRank rank = _groupRanks[groupID][rankID];
        if (rank.order == 0) {
            // we don't copy the initial permissions to the owner.
            continue;
        }
        permissionsRows[nameKey]->setAll(false);
        *(permissionsRows[nameKey]) |= *perms;
    }
}

void DomainServerSettingsManager::packPermissionsForMap(QString mapName,
                                                        NodePermissionsMap& permissionsRows,
                                                        QString keyPath) {
    // find (or create) the "security" section of the settings map
    QVariant* security = _configMap.valueForKeyPath("security", true);
    if (!security->canConvert(QMetaType::QVariantMap)) {
        (*security) = QVariantMap();
    }

    // find (or create) whichever subsection of "security" we are packing
    QVariant* permissions = _configMap.valueForKeyPath(keyPath, true);
    if (!permissions->canConvert(QMetaType::QVariantList)) {
        (*permissions) = QVariantList();
    }

    // convert details for each member of the subsection
    QVariantList* permissionsList = reinterpret_cast<QVariantList*>(permissions);
    (*permissionsList).clear();
    QList<NodePermissionsKey> permissionsKeys = permissionsRows.keys();

    // when a group is added from the domain-server settings page, the config map has a group-name with
    // no ID or rank.  We need to leave that there until we get a valid response back from the api.
    // once we have the ranks and IDs, we need to delete the original entry so that it doesn't show
    // up in the settings-page with undefined's after it.
    QHash<QString, bool> groupNamesWithRanks;
    // note which groups have rank/ID information
    foreach (NodePermissionsKey userKey, permissionsKeys) {
        NodePermissionsPointer perms = permissionsRows[userKey];
        if (perms->getRankID() != QUuid()) {
            groupNamesWithRanks[userKey.first] = true;
        }
    }
    foreach (NodePermissionsKey userKey, permissionsKeys) {
        NodePermissionsPointer perms = permissionsRows[userKey];
        if (perms->isGroup()) {
            QString groupName = userKey.first;
            if (perms->getRankID() == QUuid() && groupNamesWithRanks.contains(groupName)) {
                // copy the values from this user-added entry to the other (non-owner) ranks and remove it.
                permissionsRows.remove(userKey);
                initializeGroupPermissions(permissionsRows, groupName, perms);
            }
        }
    }

    // convert each group-name / rank-id pair to a variant-map
    foreach (NodePermissionsKey userKey, permissionsKeys) {
        if (!permissionsRows.contains(userKey)) {
            continue;
        }
        NodePermissionsPointer perms = permissionsRows[userKey];
        if (perms->isGroup()) {
            QHash<QUuid, GroupRank>& groupRanks = _groupRanks[perms->getGroupID()];
            *permissionsList += perms->toVariant(groupRanks);
        } else {
            *permissionsList += perms->toVariant();
        }
    }
}

void DomainServerSettingsManager::packPermissions() {
    // transfer details from _agentPermissions to _configMap

    // save settings for anonymous / logged-in / localhost
    packPermissionsForMap("standard_permissions", _standardAgentPermissions, AGENT_STANDARD_PERMISSIONS_KEYPATH);

    // save settings for specific users
    packPermissionsForMap("permissions", _agentPermissions, AGENT_PERMISSIONS_KEYPATH);

    // save settings for IP addresses
    packPermissionsForMap("permissions", _ipPermissions, IP_PERMISSIONS_KEYPATH);

    // save settings for MAC addresses
    packPermissionsForMap("permissions", _macPermissions, MAC_PERMISSIONS_KEYPATH);

    // save settings for groups
    packPermissionsForMap("permissions", _groupPermissions, GROUP_PERMISSIONS_KEYPATH);

    // save settings for blacklist groups
    packPermissionsForMap("permissions", _groupForbiddens, GROUP_FORBIDDENS_KEYPATH);

    persistToFile();
}

bool DomainServerSettingsManager::unpackPermissionsForKeypath(const QString& keyPath,
                                                              NodePermissionsMap* mapPointer,
                                                              std::function<void(NodePermissionsPointer)> customUnpacker) {

    mapPointer->clear();

    QVariant* permissions = _configMap.valueForKeyPath(keyPath, true);
    if (!permissions->canConvert(QMetaType::QVariantList)) {
        qDebug() << "Failed to extract permissions for key path" << keyPath << "from settings.";
        (*permissions) = QVariantList();
    }

    bool needPack = false;

    QList<QVariant> permissionsList = permissions->toList();
    foreach (QVariant permsHash, permissionsList) {
        NodePermissionsPointer perms { new NodePermissions(permsHash.toMap()) };
        QString id = perms->getID();
        
        NodePermissionsKey idKey = perms->getKey();

        if (mapPointer->contains(idKey)) {
            qDebug() << "Duplicate name in permissions table for" << keyPath << " - " << id;
            *((*mapPointer)[idKey]) |= *perms;
            needPack = true;
        } else {
            (*mapPointer)[idKey] = perms;
        }

        if (customUnpacker) {
            customUnpacker(perms);
        }
    }

    return needPack;

}

void DomainServerSettingsManager::unpackPermissions() {
    // transfer details from _configMap to _agentPermissions

    bool needPack = false;

    needPack |= unpackPermissionsForKeypath(AGENT_STANDARD_PERMISSIONS_KEYPATH, &_standardAgentPermissions);

    needPack |= unpackPermissionsForKeypath(AGENT_PERMISSIONS_KEYPATH, &_agentPermissions);

    needPack |= unpackPermissionsForKeypath(IP_PERMISSIONS_KEYPATH, &_ipPermissions,
        [&](NodePermissionsPointer perms){
            // make sure that this permission row is for a valid IP address
            if (QHostAddress(perms->getKey().first).isNull()) {
                _ipPermissions.remove(perms->getKey());

                // we removed a row from the IP permissions, we'll need a re-pack
                needPack = true;
            }
    });

    needPack |= unpackPermissionsForKeypath(MAC_PERMISSIONS_KEYPATH, &_macPermissions,
        [&](NodePermissionsPointer perms){
            // make sure that this permission row is for a non-empty hardware
            if (perms->getKey().first.isEmpty()) {
                _macPermissions.remove(perms->getKey());
                
                // we removed a row from the MAC permissions, we'll need a re-pack
                needPack = true;
            }
    });


    needPack |= unpackPermissionsForKeypath(GROUP_PERMISSIONS_KEYPATH, &_groupPermissions,
        [&](NodePermissionsPointer perms){
            if (perms->isGroup()) {
                // the group-id was cached.  hook-up the uuid in the uuid->group hash
                _groupPermissionsByUUID[GroupByUUIDKey(perms->getGroupID(), perms->getRankID())] = _groupPermissions[perms->getKey()];
                needPack |= setGroupID(perms->getID(), perms->getGroupID());
            }
    });

    needPack |= unpackPermissionsForKeypath(GROUP_FORBIDDENS_KEYPATH, &_groupForbiddens,
        [&](NodePermissionsPointer perms) {
            if (perms->isGroup()) {
                // the group-id was cached.  hook-up the uuid in the uuid->group hash
                _groupForbiddensByUUID[GroupByUUIDKey(perms->getGroupID(), perms->getRankID())] = _groupForbiddens[perms->getKey()];
                needPack |= setGroupID(perms->getID(), perms->getGroupID());
            }
    });

    // if any of the standard names are missing, add them
    foreach(const QString& standardName, NodePermissions::standardNames) {
        NodePermissionsKey standardKey { standardName, 0 };
        if (!_standardAgentPermissions.contains(standardKey)) {
            // we don't have permissions for one of the standard groups, so we'll add them now
            NodePermissionsPointer perms { new NodePermissions(standardKey) };

            if (standardKey == NodePermissions::standardNameLocalhost) {
                // the localhost user is granted all permissions by default
                perms->setAll(true);
            } else {
                // anonymous, logged in, and friend users get connect permissions by default
                perms->set(NodePermissions::Permission::canConnectToDomain);
            }

            // add the permissions to the standard map
            _standardAgentPermissions[standardKey] = perms;

            // this will require a packing of permissions
            needPack = true;
        }
    }

    needPack |= ensurePermissionsForGroupRanks();

    if (needPack) {
        packPermissions();
    }

    #ifdef WANT_DEBUG
    qDebug() << "--------------- permissions ---------------------";
    QList<QHash<NodePermissionsKey, NodePermissionsPointer>> permissionsSets;
    permissionsSets << _standardAgentPermissions.get() << _agentPermissions.get()
                    << _groupPermissions.get() << _groupForbiddens.get()
                    << _ipPermissions.get() << _macPermissions.get();
    foreach (auto permissionSet, permissionsSets) {
        QHashIterator<NodePermissionsKey, NodePermissionsPointer> i(permissionSet);
        while (i.hasNext()) {
            i.next();
            NodePermissionsPointer perms = i.value();
            if (perms->isGroup()) {
                qDebug() << i.key() << perms->getGroupID() << perms;
            } else {
                qDebug() << i.key() << perms;
            }
        }
    }
    #endif
}

bool DomainServerSettingsManager::ensurePermissionsForGroupRanks() {
    // make sure each rank in each group has its own set of permissions
    bool changed = false;
    QList<QUuid> permissionGroupIDs = getGroupIDs();
    foreach (QUuid groupID, permissionGroupIDs) {
        QString groupName = _groupNames[groupID];
        QHash<QUuid, GroupRank>& ranksForGroup = _groupRanks[groupID];
        foreach (QUuid rankID, ranksForGroup.keys()) {
            NodePermissionsKey nameKey = NodePermissionsKey(groupName, rankID);
            GroupByUUIDKey idKey = GroupByUUIDKey(groupID, rankID);
            NodePermissionsPointer perms;
            if (_groupPermissions.contains(nameKey)) {
                perms = _groupPermissions[nameKey];
            } else {
                perms = NodePermissionsPointer(new NodePermissions(nameKey));
                _groupPermissions[nameKey] = perms;
                changed = true;
            }
            if (perms->getGroupID() != groupID) {
                perms->setGroupID(groupID);
                changed = true;
            }
            if (perms->getRankID() != rankID) {
                perms->setRankID(rankID);
                changed = true;
            }
            _groupPermissionsByUUID[idKey] = perms;
        }
    }

    QList<QUuid> forbiddenGroupIDs = getBlacklistGroupIDs();
    foreach (QUuid groupID, forbiddenGroupIDs) {
        QString groupName = _groupNames[groupID];
        QHash<QUuid, GroupRank>& ranksForGroup = _groupRanks[groupID];
        foreach (QUuid rankID, ranksForGroup.keys()) {
            NodePermissionsKey nameKey = NodePermissionsKey(groupName, rankID);
            GroupByUUIDKey idKey = GroupByUUIDKey(groupID, rankID);
            NodePermissionsPointer perms;
            if (_groupForbiddens.contains(nameKey)) {
                perms = _groupForbiddens[nameKey];
            } else {
                perms = NodePermissionsPointer(new NodePermissions(nameKey));
                _groupForbiddens[nameKey] = perms;
                changed = true;
            }
            if (perms->getGroupID() != groupID) {
                perms->setGroupID(groupID);
                changed = true;
            }
            if (perms->getRankID() != rankID) {
                perms->setRankID(rankID);
                changed = true;
            }
            _groupForbiddensByUUID[idKey] = perms;
        }
    }

    return changed;
}

void DomainServerSettingsManager::processNodeKickRequestPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    // before we do any processing on this packet make sure it comes from a node that is allowed to kick
    if (sendingNode->getCanKick()) {
        // pull the UUID being kicked from the packet
        QUuid nodeUUID = QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));

        if (!nodeUUID.isNull() && nodeUUID != sendingNode->getUUID()) {
            // make sure we actually have a node with this UUID
            auto limitedNodeList = DependencyManager::get<LimitedNodeList>();

            auto matchingNode = limitedNodeList->nodeWithUUID(nodeUUID);

            if (matchingNode) {
                // we have a matching node, time to decide how to store updated permissions for this node

                NodePermissionsPointer destinationPermissions;

                auto verifiedUsername = matchingNode->getPermissions().getVerifiedUserName();

                bool newPermissions = false;

                if (!verifiedUsername.isEmpty()) {
                    // if we have a verified user name for this user, we apply the kick to the username

                    // check if there were already permissions
                    bool hadPermissions = havePermissionsForName(verifiedUsername);

                    // grab or create permissions for the given username
                    auto userPermissions = _agentPermissions[matchingNode->getPermissions().getKey()];

                    newPermissions = !hadPermissions || userPermissions->can(NodePermissions::Permission::canConnectToDomain);

                    // ensure that the connect permission is clear
                    userPermissions->clear(NodePermissions::Permission::canConnectToDomain);
                } else {
                    // otherwise we apply the kick to the IP from active socket for this node and the MAC address

                    // remove connect permissions for the IP (falling back to the public socket if not yet active)
                    auto& kickAddress = matchingNode->getActiveSocket()
                        ? matchingNode->getActiveSocket()->getAddress()
                        : matchingNode->getPublicSocket().getAddress();

                    NodePermissionsKey ipAddressKey(kickAddress.toString(), QUuid());

                    // check if there were already permissions for the IP
                    bool hadIPPermissions = hasPermissionsForIP(kickAddress);

                    // grab or create permissions for the given IP address
                    auto ipPermissions = _ipPermissions[ipAddressKey];

                    if (!hadIPPermissions || ipPermissions->can(NodePermissions::Permission::canConnectToDomain)) {
                        newPermissions = true;

                        ipPermissions->clear(NodePermissions::Permission::canConnectToDomain);
                    }

                    // potentially remove connect permissions for the MAC address
                    DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(matchingNode->getLinkedData());
                    if (nodeData) {
                        NodePermissionsKey macAddressKey(nodeData->getHardwareAddress(), 0);

                        bool hadMACPermissions = hasPermissionsForMAC(nodeData->getHardwareAddress());

                        auto macPermissions = _macPermissions[macAddressKey];

                        if (!hadMACPermissions || macPermissions->can(NodePermissions::Permission::canConnectToDomain)) {
                            newPermissions = true;

                            macPermissions->clear(NodePermissions::Permission::canConnectToDomain);
                        }
                    }
                }

                if (newPermissions) {
                    qDebug() << "Removing connect permission for node" << uuidStringWithoutCurlyBraces(matchingNode->getUUID())
                        << "after kick request from" << uuidStringWithoutCurlyBraces(sendingNode->getUUID());

                    // we've changed permissions, time to store them to disk and emit our signal to say they have changed
                    packPermissions();
                } else {
                    emit updateNodePermissions();
                }

            } else {
                qWarning() << "Node kick request received for unknown node. Refusing to process.";
            }
        } else {
            // this isn't a UUID we can use
            qWarning() << "Node kick request received for invalid node ID or from node being kicked. Refusing to process.";
        }

    } else {
        qWarning() << "Refusing to process a kick packet from node" << uuidStringWithoutCurlyBraces(sendingNode->getUUID())
        << "that does not have kick permissions.";
    }
}

QStringList DomainServerSettingsManager::getAllNames() const {
    QStringList result;
    foreach (auto key, _agentPermissions.keys()) {
        result << key.first.toLower();
    }
    return result;
}

NodePermissions DomainServerSettingsManager::getStandardPermissionsForName(const NodePermissionsKey& name) const {
    if (_standardAgentPermissions.contains(name)) {
        return *(_standardAgentPermissions[name].get());
    }
    NodePermissions nullPermissions;
    nullPermissions.setAll(false);
    return nullPermissions;
}

NodePermissions DomainServerSettingsManager::getPermissionsForName(const QString& name) const {
    NodePermissionsKey nameKey = NodePermissionsKey(name, 0);
    if (_agentPermissions.contains(nameKey)) {
        return *(_agentPermissions[nameKey].get());
    }
    NodePermissions nullPermissions;
    nullPermissions.setAll(false);
    return nullPermissions;
}

NodePermissions DomainServerSettingsManager::getPermissionsForIP(const QHostAddress& address) const {
    NodePermissionsKey ipKey = NodePermissionsKey(address.toString(), 0);
    if (_ipPermissions.contains(ipKey)) {
        return *(_ipPermissions[ipKey].get());
    }
    NodePermissions nullPermissions;
    nullPermissions.setAll(false);
    return nullPermissions;
}

NodePermissions DomainServerSettingsManager::getPermissionsForMAC(const QString& macAddress) const {
    NodePermissionsKey macKey = NodePermissionsKey(macAddress, 0);
    if (_macPermissions.contains(macKey)) {
        return *(_macPermissions[macKey].get());
    }
    NodePermissions nullPermissions;
    nullPermissions.setAll(false);
    return nullPermissions;
}

NodePermissions DomainServerSettingsManager::getPermissionsForGroup(const QString& groupName, QUuid rankID) const {
    NodePermissionsKey groupRankKey = NodePermissionsKey(groupName, rankID);
    if (_groupPermissions.contains(groupRankKey)) {
        return *(_groupPermissions[groupRankKey].get());
    }
    NodePermissions nullPermissions;
    nullPermissions.setAll(false);
    return nullPermissions;
}

NodePermissions DomainServerSettingsManager::getPermissionsForGroup(const QUuid& groupID, QUuid rankID) const {
    GroupByUUIDKey byUUIDKey = GroupByUUIDKey(groupID, rankID);
    if (!_groupPermissionsByUUID.contains(byUUIDKey)) {
        NodePermissions nullPermissions;
        nullPermissions.setAll(false);
        return nullPermissions;
    }
    NodePermissionsKey groupKey = _groupPermissionsByUUID[byUUIDKey]->getKey();
    return getPermissionsForGroup(groupKey.first, groupKey.second);
}

NodePermissions DomainServerSettingsManager::getForbiddensForGroup(const QString& groupName, QUuid rankID) const {
    NodePermissionsKey groupRankKey = NodePermissionsKey(groupName, rankID);
    if (_groupForbiddens.contains(groupRankKey)) {
        return *(_groupForbiddens[groupRankKey].get());
    }
    NodePermissions allForbiddens;
    allForbiddens.setAll(true);
    return allForbiddens;
}

NodePermissions DomainServerSettingsManager::getForbiddensForGroup(const QUuid& groupID, QUuid rankID) const {
    GroupByUUIDKey byUUIDKey = GroupByUUIDKey(groupID, rankID);
    if (!_groupForbiddensByUUID.contains(byUUIDKey)) {
        NodePermissions allForbiddens;
        allForbiddens.setAll(true);
        return allForbiddens;
    }

    NodePermissionsKey groupKey = _groupForbiddensByUUID[byUUIDKey]->getKey();
    return getForbiddensForGroup(groupKey.first, groupKey.second);
}

QVariant DomainServerSettingsManager::valueOrDefaultValueForKeyPath(const QString& keyPath) {
    const QVariant* foundValue = _configMap.valueForKeyPath(keyPath);

    if (foundValue) {
        return *foundValue;
    } else {
        int dotIndex = keyPath.indexOf('.');

        QString groupKey = keyPath.mid(0, dotIndex);
        QString settingKey = keyPath.mid(dotIndex + 1);

        foreach(const QVariant& group, _descriptionArray.toVariantList()) {
            QVariantMap groupMap = group.toMap();

            if (groupMap[DESCRIPTION_NAME_KEY].toString() == groupKey) {
                foreach(const QVariant& setting, groupMap[DESCRIPTION_SETTINGS_KEY].toList()) {
                    QVariantMap settingMap = setting.toMap();
                    if (settingMap[DESCRIPTION_NAME_KEY].toString() == settingKey) {
                        return settingMap[SETTING_DEFAULT_KEY];
                    }
                }

                return QVariant();
            }
        }
    }

    return QVariant();
}

bool DomainServerSettingsManager::handlePublicHTTPRequest(HTTPConnection* connection, const QUrl &url) {
    if (connection->requestOperation() == QNetworkAccessManager::GetOperation && url.path() == SETTINGS_PATH_JSON) {
        // this is a GET operation for our settings

        // check if there is a query parameter for settings affecting a particular type of assignment
        const QString SETTINGS_TYPE_QUERY_KEY = "type";
        QUrlQuery settingsQuery(url);
        QString typeValue = settingsQuery.queryItemValue(SETTINGS_TYPE_QUERY_KEY);

        if (!typeValue.isEmpty()) {
            QJsonObject responseObject = responseObjectForType(typeValue);

            connection->respond(HTTPConnection::StatusCode200, QJsonDocument(responseObject).toJson(), "application/json");

            return true;
        } else {
            return false;
        }
    }

    return false;
}

bool DomainServerSettingsManager::handleAuthenticatedHTTPRequest(HTTPConnection *connection, const QUrl &url) {
    if (connection->requestOperation() == QNetworkAccessManager::PostOperation && url.path() == SETTINGS_PATH_JSON) {
        // this is a POST operation to change one or more settings
        QJsonDocument postedDocument = QJsonDocument::fromJson(connection->requestContent());
        QJsonObject postedObject = postedDocument.object();

        // we recurse one level deep below each group for the appropriate setting
        bool restartRequired = recurseJSONObjectAndOverwriteSettings(postedObject);

        // store whatever the current _settingsMap is to file
        persistToFile();

        // return success to the caller
        QString jsonSuccess = "{\"status\": \"success\"}";
        connection->respond(HTTPConnection::StatusCode200, jsonSuccess.toUtf8(), "application/json");

        // defer a restart to the domain-server, this gives our HTTPConnection enough time to respond
        if (restartRequired) {
            const int DOMAIN_SERVER_RESTART_TIMER_MSECS = 1000;
            QTimer::singleShot(DOMAIN_SERVER_RESTART_TIMER_MSECS, qApp, SLOT(restart()));
        } else {
            unpackPermissions();
            apiRefreshGroupInformation();
            emit updateNodePermissions();
        }

        return true;
    } else if (connection->requestOperation() == QNetworkAccessManager::GetOperation && url.path() == SETTINGS_PATH_JSON) {
        // setup a JSON Object with descriptions and non-omitted settings
        const QString SETTINGS_RESPONSE_DESCRIPTION_KEY = "descriptions";
        const QString SETTINGS_RESPONSE_VALUE_KEY = "values";

        QJsonObject rootObject;
        rootObject[SETTINGS_RESPONSE_DESCRIPTION_KEY] = _descriptionArray;
        rootObject[SETTINGS_RESPONSE_VALUE_KEY] = responseObjectForType("", true);
        connection->respond(HTTPConnection::StatusCode200, QJsonDocument(rootObject).toJson(), "application/json");
    }

    return false;
}

QJsonObject DomainServerSettingsManager::responseObjectForType(const QString& typeValue, bool isAuthenticated) {
    QJsonObject responseObject;

    if (!typeValue.isEmpty() || isAuthenticated) {
        // convert the string type value to a QJsonValue
        QJsonValue queryType = typeValue.isEmpty() ? QJsonValue() : QJsonValue(typeValue.toInt());

        const QString AFFECTED_TYPES_JSON_KEY = "assignment-types";

        // enumerate the groups in the description object to find which settings to pass
        foreach(const QJsonValue& groupValue, _descriptionArray) {
            QJsonObject groupObject = groupValue.toObject();
            QString groupKey = groupObject[DESCRIPTION_NAME_KEY].toString();
            QJsonArray groupSettingsArray = groupObject[DESCRIPTION_SETTINGS_KEY].toArray();

            QJsonObject groupResponseObject;

            foreach(const QJsonValue& settingValue, groupSettingsArray) {
                const QString VALUE_HIDDEN_FLAG_KEY = "value-hidden";

                QJsonObject settingObject = settingValue.toObject();

                if (!settingObject[VALUE_HIDDEN_FLAG_KEY].toBool()) {
                    QJsonArray affectedTypesArray = settingObject[AFFECTED_TYPES_JSON_KEY].toArray();
                    if (affectedTypesArray.isEmpty()) {
                        affectedTypesArray = groupObject[AFFECTED_TYPES_JSON_KEY].toArray();
                    }

                    if (affectedTypesArray.contains(queryType) ||
                        (queryType.isNull() && isAuthenticated)) {
                        // this is a setting we should include in the responseObject

                        QString settingName = settingObject[DESCRIPTION_NAME_KEY].toString();

                        // we need to check if the settings map has a value for this setting
                        QVariant variantValue;

                        if (!groupKey.isEmpty()) {
                             QVariant settingsMapGroupValue = _configMap.value(groupKey);

                            if (!settingsMapGroupValue.isNull()) {
                                variantValue = settingsMapGroupValue.toMap().value(settingName);
                            }
                        } else {
                            variantValue = _configMap.value(settingName);
                        }

                        QJsonValue result;

                        if (variantValue.isNull()) {
                            // no value for this setting, pass the default
                            if (settingObject.contains(SETTING_DEFAULT_KEY)) {
                                result = settingObject[SETTING_DEFAULT_KEY];
                            } else {
                                // users are allowed not to provide a default for string values
                                // if so we set to the empty string
                                result = QString("");
                            }

                        } else {
                            result = QJsonValue::fromVariant(variantValue);
                        }

                        if (!groupKey.isEmpty()) {
                            // this belongs in the group object
                            groupResponseObject[settingName] = result;
                        } else {
                            // this is a value that should be at the root
                            responseObject[settingName] = result;
                        }
                    }
                }
            }

            if (!groupKey.isEmpty() && !groupResponseObject.isEmpty()) {
                // set this group's object to the constructed object
                responseObject[groupKey] = groupResponseObject;
            }
        }
    }


    return responseObject;
}

void DomainServerSettingsManager::updateSetting(const QString& key, const QJsonValue& newValue, QVariantMap& settingMap,
                                                const QJsonObject& settingDescription) {
    if (newValue.isString()) {
        if (newValue.toString().isEmpty()) {
            // this is an empty value, clear it in settings variant so the default is sent
            settingMap.remove(key);
        } else {
            // make sure the resulting json value has the right type
            QString settingType = settingDescription[SETTING_DESCRIPTION_TYPE_KEY].toString();
            const QString INPUT_DOUBLE_TYPE = "double";
            const QString INPUT_INTEGER_TYPE = "int";

            if (settingType == INPUT_DOUBLE_TYPE) {
                settingMap[key] = newValue.toString().toDouble();
            } else if (settingType == INPUT_INTEGER_TYPE) {
                settingMap[key] = newValue.toString().toInt();
            } else {
                QString sanitizedValue = newValue.toString();

                // we perform special handling for viewpoints here
                // we do not want them to be prepended with a slash
                if (key == SETTINGS_VIEWPOINT_KEY && !sanitizedValue.startsWith('/')) {
                    sanitizedValue.prepend('/');
                }

                settingMap[key] = sanitizedValue;
            }
        }
    } else if (newValue.isBool()) {
        settingMap[key] = newValue.toBool();
    } else if (newValue.isObject()) {
        if (!settingMap.contains(key)) {
            // we don't have a map below this key yet, so set it up now
            settingMap[key] = QVariantMap();
        }

        QVariant& possibleMap = settingMap[key];

        if (!possibleMap.canConvert(QMetaType::QVariantMap)) {
            // if this isn't a map then we need to make it one, otherwise we're about to crash
            qDebug() << "Value at" << key << "was not the expected QVariantMap while updating DS settings"
                << "- removing existing value and making it a QVariantMap";
            possibleMap = QVariantMap();
        }

        QVariantMap& thisMap = *reinterpret_cast<QVariantMap*>(possibleMap.data());
        foreach(const QString childKey, newValue.toObject().keys()) {

            QJsonObject childDescriptionObject = settingDescription;

            // is this the key? if so we have the description already
            if (key != settingDescription[DESCRIPTION_NAME_KEY].toString()) {
                // otherwise find the description object for this childKey under columns
                foreach(const QJsonValue& column, settingDescription[DESCRIPTION_COLUMNS_KEY].toArray()) {
                    if (column.isObject()) {
                        QJsonObject thisDescription = column.toObject();
                        if (thisDescription[DESCRIPTION_NAME_KEY] == childKey) {
                            childDescriptionObject = column.toObject();
                            break;
                        }
                    }
                }
            }

            QString sanitizedKey = childKey;

            if (key == SETTINGS_PATHS_KEY && !sanitizedKey.startsWith('/')) {
                // We perform special handling for paths here.
                // If we got sent a path without a leading slash then we add it.
                sanitizedKey.prepend("/");
            }

            updateSetting(sanitizedKey, newValue.toObject()[childKey], thisMap, childDescriptionObject);
        }

        if (settingMap[key].toMap().isEmpty()) {
            // we've cleared all of the settings below this value, so remove this one too
            settingMap.remove(key);
        }
    } else if (newValue.isArray()) {
        // we just assume array is replacement
        // TODO: we still need to recurse here with the description in case values in the array have special types
        settingMap[key] = newValue.toArray().toVariantList();
    }

    sortPermissions();
}

QJsonObject DomainServerSettingsManager::settingDescriptionFromGroup(const QJsonObject& groupObject,
                                                                     const QString& settingName) {
    foreach(const QJsonValue& settingValue, groupObject[DESCRIPTION_SETTINGS_KEY].toArray()) {
        QJsonObject settingObject = settingValue.toObject();
        if (settingObject[DESCRIPTION_NAME_KEY].toString() == settingName) {
            return settingObject;
        }
    }

    return QJsonObject();
}

bool DomainServerSettingsManager::recurseJSONObjectAndOverwriteSettings(const QJsonObject& postedObject) {
    static const QString SECURITY_ROOT_KEY = "security";
    static const QString AC_SUBNET_WHITELIST_KEY = "ac_subnet_whitelist";

    auto& settingsVariant = _configMap.getConfig();
    bool needRestart = false;

    // Iterate on the setting groups
    foreach(const QString& rootKey, postedObject.keys()) {
        QJsonValue rootValue = postedObject[rootKey];

        if (!settingsVariant.contains(rootKey)) {
            // we don't have a map below this key yet, so set it up now
            settingsVariant[rootKey] = QVariantMap();
        }

        QVariantMap* thisMap = &settingsVariant;

        QJsonObject groupDescriptionObject;

        // we need to check the description array to see if this is a root setting or a group setting
        foreach(const QJsonValue& groupValue, _descriptionArray) {
            if (groupValue.toObject()[DESCRIPTION_NAME_KEY] == rootKey) {
                // we matched a group - keep this since we'll use it below to update the settings
                groupDescriptionObject = groupValue.toObject();

                // change the map we will update to be the map for this group
                thisMap = reinterpret_cast<QVariantMap*>(settingsVariant[rootKey].data());

                break;
            }
        }

        if (groupDescriptionObject.isEmpty()) {
            // this is a root value, so we can call updateSetting for it directly
            // first we need to find our description value for it

            QJsonObject matchingDescriptionObject;

            foreach(const QJsonValue& groupValue, _descriptionArray) {
                // find groups with root values (they don't have a group name)
                QJsonObject groupObject = groupValue.toObject();
                if (!groupObject.contains(DESCRIPTION_NAME_KEY)) {
                    // this is a group with root values - check if our setting is in here
                    matchingDescriptionObject = settingDescriptionFromGroup(groupObject, rootKey);

                    if (!matchingDescriptionObject.isEmpty()) {
                        break;
                    }
                }
            }

            if (!matchingDescriptionObject.isEmpty()) {
                updateSetting(rootKey, rootValue, *thisMap, matchingDescriptionObject);
                if (rootKey != SECURITY_ROOT_KEY) {
                    needRestart = true;
                }
            } else {
                qDebug() << "Setting for root key" << rootKey << "does not exist - cannot update setting.";
            }
        } else {
            // this is a group - iterate on the settings in the group
            foreach(const QString& settingKey, rootValue.toObject().keys()) {
                // make sure this particular setting exists and we have a description object for it
                QJsonObject matchingDescriptionObject = settingDescriptionFromGroup(groupDescriptionObject, settingKey);

                // if we matched the setting then update the value
                if (!matchingDescriptionObject.isEmpty()) {
                    QJsonValue settingValue = rootValue.toObject()[settingKey];
                    updateSetting(settingKey, settingValue, *thisMap, matchingDescriptionObject);
                    if (rootKey != SECURITY_ROOT_KEY || settingKey == AC_SUBNET_WHITELIST_KEY) {
                        needRestart = true;
                    }
                } else {
                    qDebug() << "Could not find description for setting" << settingKey << "in group" << rootKey <<
                        "- cannot update setting.";
                }
            }
        }

        if (settingsVariant[rootKey].toMap().empty()) {
            // we've cleared all of the settings below this value, so remove this one too
            settingsVariant.remove(rootKey);
        }
    }

    // re-merge the user and master configs after a settings change
    _configMap.mergeMasterAndUserConfigs();

    return needRestart;
}

// Compare two members of a permissions list
bool permissionVariantLessThan(const QVariant &v1, const QVariant &v2) {
    if (!v1.canConvert(QMetaType::QVariantMap) ||
        !v2.canConvert(QMetaType::QVariantMap)) {
        return v1.toString() < v2.toString();
    }
    QVariantMap m1 = v1.toMap();
    QVariantMap m2 = v2.toMap();

    if (!m1.contains("permissions_id") ||
        !m2.contains("permissions_id")) {
        return v1.toString() < v2.toString();
    }

    if (m1.contains("rank_order") && m2.contains("rank_order") &&
        m1["permissions_id"].toString() == m2["permissions_id"].toString()) {
        return m1["rank_order"].toInt() < m2["rank_order"].toInt();
    }

    return m1["permissions_id"].toString() < m2["permissions_id"].toString();
}

void DomainServerSettingsManager::sortPermissions() {
    // sort the permission-names
    QVariant* standardPermissions = _configMap.valueForKeyPath(AGENT_STANDARD_PERMISSIONS_KEYPATH);
    if (standardPermissions && standardPermissions->canConvert(QMetaType::QVariantList)) {
        QList<QVariant>* standardPermissionsList = reinterpret_cast<QVariantList*>(standardPermissions);
        std::sort((*standardPermissionsList).begin(), (*standardPermissionsList).end(), permissionVariantLessThan);
    }
    QVariant* permissions = _configMap.valueForKeyPath(AGENT_PERMISSIONS_KEYPATH);
    if (permissions && permissions->canConvert(QMetaType::QVariantList)) {
        QList<QVariant>* permissionsList = reinterpret_cast<QVariantList*>(permissions);
        std::sort((*permissionsList).begin(), (*permissionsList).end(), permissionVariantLessThan);
    }
    QVariant* groupPermissions = _configMap.valueForKeyPath(GROUP_PERMISSIONS_KEYPATH);
    if (groupPermissions && groupPermissions->canConvert(QMetaType::QVariantList)) {
        QList<QVariant>* permissionsList = reinterpret_cast<QVariantList*>(groupPermissions);
        std::sort((*permissionsList).begin(), (*permissionsList).end(), permissionVariantLessThan);
    }
    QVariant* forbiddenPermissions = _configMap.valueForKeyPath(GROUP_FORBIDDENS_KEYPATH);
    if (forbiddenPermissions && forbiddenPermissions->canConvert(QMetaType::QVariantList)) {
        QList<QVariant>* permissionsList = reinterpret_cast<QVariantList*>(forbiddenPermissions);
        std::sort((*permissionsList).begin(), (*permissionsList).end(), permissionVariantLessThan);
    }
}

void DomainServerSettingsManager::persistToFile() {
    sortPermissions();

    // make sure we have the dir the settings file is supposed to live in
    QFileInfo settingsFileInfo(_configMap.getUserConfigFilename());

    if (!settingsFileInfo.dir().exists()) {
        settingsFileInfo.dir().mkpath(".");
    }

    QFile settingsFile(_configMap.getUserConfigFilename());

    if (settingsFile.open(QIODevice::WriteOnly)) {
        settingsFile.write(QJsonDocument::fromVariant(_configMap.getConfig()).toJson());
    } else {
        qCritical("Could not write to JSON settings file. Unable to persist settings.");

        // failed to write, reload whatever the current config state is
        _configMap.loadConfig(_argumentList);
    }
}

QStringList DomainServerSettingsManager::getAllKnownGroupNames() {
    // extract all the group names from the group-permissions and group-forbiddens settings
    QSet<QString> result;

    QHashIterator<NodePermissionsKey, NodePermissionsPointer> i(_groupPermissions.get());
    while (i.hasNext()) {
        i.next();
        NodePermissionsKey key = i.key();
        result += key.first;
    }

    QHashIterator<NodePermissionsKey, NodePermissionsPointer> j(_groupForbiddens.get());
    while (j.hasNext()) {
        j.next();
        NodePermissionsKey key = j.key();
        result += key.first;
    }

    return result.toList();
}

bool DomainServerSettingsManager::setGroupID(const QString& groupName, const QUuid& groupID) {
    bool changed = false;
    _groupIDs[groupName.toLower()] = groupID;
    _groupNames[groupID] = groupName;

    QHashIterator<NodePermissionsKey, NodePermissionsPointer> i(_groupPermissions.get());
    while (i.hasNext()) {
        i.next();
        NodePermissionsPointer perms = i.value();
        if (perms->getID().toLower() == groupName.toLower() && !perms->isGroup()) {
            changed = true;
            perms->setGroupID(groupID);
        }
    }

    QHashIterator<NodePermissionsKey, NodePermissionsPointer> j(_groupForbiddens.get());
    while (j.hasNext()) {
        j.next();
        NodePermissionsPointer perms = j.value();
        if (perms->getID().toLower() == groupName.toLower() && !perms->isGroup()) {
            changed = true;
            perms->setGroupID(groupID);
        }
    }

    return changed;
}

void DomainServerSettingsManager::apiRefreshGroupInformation() {
    if (!DependencyManager::get<AccountManager>()->hasAuthEndpoint()) {
        // can't yet.
        return;
    }

    bool changed = false;

    QStringList groupNames = getAllKnownGroupNames();
    foreach (QString groupName, groupNames) {
        QString lowerGroupName = groupName.toLower();
        if (_groupIDs.contains(lowerGroupName)) {
            // we already know about this one.  recall setGroupID in case the group has been
            // added to another section (the same group is found in both groups and blacklists).
            changed = setGroupID(groupName, _groupIDs[lowerGroupName]);
            continue;
        }
        apiGetGroupID(groupName);
    }

    foreach (QUuid groupID, _groupNames.keys()) {
        apiGetGroupRanks(groupID);
    }

    changed |= ensurePermissionsForGroupRanks();

    if (changed) {
        packPermissions();
    }

    unpackPermissions();
}

void DomainServerSettingsManager::apiGetGroupID(const QString& groupName) {
    JSONCallbackParameters callbackParams;
    callbackParams.jsonCallbackReceiver = this;
    callbackParams.jsonCallbackMethod = "apiGetGroupIDJSONCallback";
    callbackParams.errorCallbackReceiver = this;
    callbackParams.errorCallbackMethod = "apiGetGroupIDErrorCallback";

    const QString GET_GROUP_ID_PATH = "api/v1/groups/names/%1";
    DependencyManager::get<AccountManager>()->sendRequest(GET_GROUP_ID_PATH.arg(groupName),
                                                          AccountManagerAuth::Required,
                                                          QNetworkAccessManager::GetOperation, callbackParams);
}

void DomainServerSettingsManager::apiGetGroupIDJSONCallback(QNetworkReply& requestReply) {
    // {
    //     "data":{
    //         "groups":[{
    //             "description":null,
    //             "id":"fd55479a-265d-4990-854e-3d04214ad1b0",
    //             "is_list":false,
    //             "membership":{
    //                 "permissions":{
    //                     "custom_1=":false,
    //                     "custom_2=":false,
    //                     "custom_3=":false,
    //                     "custom_4=":false,
    //                     "del_group=":true,
    //                     "invite_member=":true,
    //                     "kick_member=":true,
    //                     "list_members=":true,
    //                     "mv_group=":true,
    //                     "query_members=":true,
    //                     "rank_member=":true
    //                 },
    //                 "rank":{
    //                     "name=":"owner",
    //                     "order=":0
    //                 }
    //             },
    //             "name":"Blerg Blah"
    //         }]
    //     },
    //     "status":"success"
    // }
    QJsonObject jsonObject = QJsonDocument::fromJson(requestReply.readAll()).object();
    if (jsonObject["status"].toString() == "success") {
        QJsonArray groups = jsonObject["data"].toObject()["groups"].toArray();
        for (int i = 0; i < groups.size(); i++) {
            QJsonObject group = groups.at(i).toObject();
            QString groupName = group["name"].toString();
            QUuid groupID = QUuid(group["id"].toString());

            bool changed = setGroupID(groupName, groupID);
            if (changed) {
                packPermissions();
                apiGetGroupRanks(groupID);
            }
        }
    } else {
        qDebug() << "getGroupID api call returned:" << QJsonDocument(jsonObject).toJson(QJsonDocument::Compact);
    }
}

void DomainServerSettingsManager::apiGetGroupIDErrorCallback(QNetworkReply& requestReply) {
    qDebug() << "******************** getGroupID api call failed:" << requestReply.error();
}

void DomainServerSettingsManager::apiGetGroupRanks(const QUuid& groupID) {
    JSONCallbackParameters callbackParams;
    callbackParams.jsonCallbackReceiver = this;
    callbackParams.jsonCallbackMethod = "apiGetGroupRanksJSONCallback";
    callbackParams.errorCallbackReceiver = this;
    callbackParams.errorCallbackMethod = "apiGetGroupRanksErrorCallback";

    const QString GET_GROUP_RANKS_PATH = "api/v1/groups/%1/ranks";
    DependencyManager::get<AccountManager>()->sendRequest(GET_GROUP_RANKS_PATH.arg(groupID.toString().mid(1,36)),
                                                          AccountManagerAuth::Required,
                                                          QNetworkAccessManager::GetOperation, callbackParams);
}

void DomainServerSettingsManager::apiGetGroupRanksJSONCallback(QNetworkReply& requestReply) {
    // {
    //     "data":{
    //         "groups":{
    //             "d3500f49-0655-4b1b-9846-ff8dd1b03351":{
    //                 "members_count":1,
    //                 "ranks":[
    //                     {
    //                         "id":"7979b774-e7f8-436c-9df1-912f1019f32f",
    //                         "members_count":1,
    //                         "name":"owner",
    //                         "order":0,
    //                         "permissions":{
    //                             "custom_1":false,
    //                             "custom_2":false,
    //                             "custom_3":false,
    //                             "custom_4":false,
    //                             "edit_group":true,
    //                             "edit_member":true,
    //                             "edit_rank":true,
    //                             "list_members":true,
    //                             "list_permissions":true,
    //                             "list_ranks":true,
    //                             "query_member":true
    //                         }
    //                     }
    //                 ]
    //             }
    //         }
    //     },"status":"success"
    // }

    bool changed = false;
    QJsonObject jsonObject = QJsonDocument::fromJson(requestReply.readAll()).object();

    if (jsonObject["status"].toString() == "success") {
        QJsonObject groups = jsonObject["data"].toObject()["groups"].toObject();
        foreach (auto groupID, groups.keys()) {
            QJsonObject group = groups[groupID].toObject();
            QJsonArray ranks = group["ranks"].toArray();

            QHash<QUuid, GroupRank>& ranksForGroup = _groupRanks[groupID];
            QHash<QUuid, bool> idsFromThisUpdate;

            for (int rankIndex = 0; rankIndex < ranks.size(); rankIndex++) {
                QJsonObject rank = ranks[rankIndex].toObject();

                QUuid rankID = QUuid(rank["id"].toString());
                int rankOrder = rank["order"].toInt();
                QString rankName = rank["name"].toString();
                int rankMembersCount = rank["members_count"].toInt();

                GroupRank groupRank(rankID, rankOrder, rankName, rankMembersCount);

                if (ranksForGroup[rankID] != groupRank) {
                    ranksForGroup[rankID] = groupRank;
                    changed = true;
                }

                idsFromThisUpdate[rankID] = true;
            }

            // clean up any that went away
            foreach (QUuid rankID, ranksForGroup.keys()) {
                if (!idsFromThisUpdate.contains(rankID)) {
                    ranksForGroup.remove(rankID);
                }
            }
        }

        changed |= ensurePermissionsForGroupRanks();
        if (changed) {
            packPermissions();
        }
    } else {
        qDebug() << "getGroupRanks api call returned:" << QJsonDocument(jsonObject).toJson(QJsonDocument::Compact);
    }
}

void DomainServerSettingsManager::apiGetGroupRanksErrorCallback(QNetworkReply& requestReply) {
    qDebug() << "******************** getGroupRanks api call failed:" << requestReply.error();
}

void DomainServerSettingsManager::recordGroupMembership(const QString& name, const QUuid groupID, QUuid rankID) {
    if (rankID != QUuid()) {
        _groupMembership[name.toLower()][groupID] = rankID;
    } else {
        _groupMembership[name.toLower()].remove(groupID);
    }
}

QUuid DomainServerSettingsManager::isGroupMember(const QString& name, const QUuid& groupID) {
    const QHash<QUuid, QUuid>& groupsForName = _groupMembership[name.toLower()];
    if (groupsForName.contains(groupID)) {
        return groupsForName[groupID];
    }
    return QUuid();
}

QList<QUuid> DomainServerSettingsManager::getGroupIDs() {
    QSet<QUuid> result;
    foreach (NodePermissionsKey groupKey, _groupPermissions.keys()) {
        if (_groupPermissions[groupKey]->isGroup()) {
            result += _groupPermissions[groupKey]->getGroupID();
        }
    }
    return result.toList();
}

QList<QUuid> DomainServerSettingsManager::getBlacklistGroupIDs() {
    QSet<QUuid> result;
    foreach (NodePermissionsKey groupKey, _groupForbiddens.keys()) {
        if (_groupForbiddens[groupKey]->isGroup()) {
            result += _groupForbiddens[groupKey]->getGroupID();
        }
    }
    return result.toList();
}

void DomainServerSettingsManager::debugDumpGroupsState() {
    qDebug() << "--------- GROUPS ---------";

    qDebug() << "_groupPermissions:";
    foreach (NodePermissionsKey groupKey, _groupPermissions.keys()) {
        NodePermissionsPointer perms = _groupPermissions[groupKey];
        qDebug() << "|  " << groupKey << perms;
    }

    qDebug() << "_groupForbiddens:";
    foreach (NodePermissionsKey groupKey, _groupForbiddens.keys()) {
        NodePermissionsPointer perms = _groupForbiddens[groupKey];
        qDebug() << "|  " << groupKey << perms;
    }

    qDebug() << "_groupIDs:";
    foreach (QString groupName, _groupIDs.keys()) {
        qDebug() << "|  " << groupName << "==>" << _groupIDs[groupName.toLower()];
    }

    qDebug() << "_groupNames:";
    foreach (QUuid groupID, _groupNames.keys()) {
        qDebug() << "|  " << groupID << "==>" << _groupNames[groupID];
    }

    qDebug() << "_groupRanks:";
    foreach (QUuid groupID, _groupRanks.keys()) {
        QHash<QUuid, GroupRank>& ranksForGroup = _groupRanks[groupID];
        qDebug() << "|  " << groupID;
        foreach (QUuid rankID, ranksForGroup.keys()) {
            QString rankName = ranksForGroup[rankID].name;
            qDebug() << "|      " << rankID << rankName;
        }
    }

    qDebug() << "_groupMembership";
    foreach (QString userName, _groupMembership.keys()) {
        QHash<QUuid, QUuid>& groupsForUser = _groupMembership[userName.toLower()];
        QString line = "";
        foreach (QUuid groupID, groupsForUser.keys()) {
            line += " g=" + groupID.toString() + ",r=" + groupsForUser[groupID].toString();
        }
        qDebug() << "|  " << userName << line;
    }
}
